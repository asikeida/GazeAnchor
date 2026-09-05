#include "AppConfig.h"
#include "BuildInfo.h"
#include "OverlayWidget.h"
#include "SettingsWindow.h"

#ifdef HAVE_KGLOBALACCEL
#include <KGlobalAccel>
#endif

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QLibraryInfo>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QScreen>
#include <QSocketNotifier>
#include <QStandardPaths>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTextStream>
#include <QVector>

#include <cerrno>
#include <cstring>

#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

namespace {

QString backendMode(const QString &platform)
{
    if (platform == QStringLiteral("xcb")) {
#ifdef HAVE_X11_FALLBACK
        return QStringLiteral("X11 fallback");
#else
        return QStringLiteral("X11 without fallback support");
#endif
    }

    if (platform != QStringLiteral("wayland")) {
        return QStringLiteral("Non-display backend (%1)").arg(platform);
    }

#ifdef HAVE_LAYER_SHELL_QT
    const QString desktop = qEnvironmentVariable("XDG_CURRENT_DESKTOP");
    if (desktop.contains(QStringLiteral("KDE"), Qt::CaseInsensitive)) {
        return QStringLiteral("Wayland LayerShellQt");
    }
    return QStringLiteral("Wayland without verified layer-shell desktop");
#else
    return QStringLiteral("Wayland without LayerShellQt");
#endif
}

QString commandServerName()
{
    QString name = QStringLiteral(APP_ID) + QStringLiteral(".ipc");
    const QString suffix = qEnvironmentVariable("GAZEANCHOR_IPC_SUFFIX");
    if (!suffix.isEmpty()) {
        name += QStringLiteral(".") + suffix;
    }
    return name;
}

QString commandServerPath()
{
    return QDir::temp().filePath(commandServerName());
}

bool sendCommandToPrimary(const QString &command, QString *errorMessage = nullptr)
{
    const QByteArray socketPath = QFile::encodeName(commandServerPath());
    const int socketFd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (socketFd == -1) {
        if (errorMessage) {
            *errorMessage = QString::fromLocal8Bit(std::strerror(errno));
        }
        return false;
    }

    sockaddr_un address {};
    address.sun_family = AF_UNIX;
    std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", socketPath.constData());

    if (::connect(socketFd, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1) {
        if (errorMessage) {
            *errorMessage = QString::fromLocal8Bit(std::strerror(errno));
        }
        ::close(socketFd);
        return false;
    }

    const QByteArray payload = command.toUtf8();
    const ssize_t written = ::write(socketFd, payload.constData(), static_cast<size_t>(payload.size()));
    if (written != payload.size()) {
        if (errorMessage) {
            *errorMessage = QString::fromLocal8Bit(std::strerror(errno));
        }
        ::close(socketFd);
        return false;
    }

    ::close(socketFd);
    return true;
}

int createCommandServer(QString *errorMessage = nullptr)
{
    const QString socketPathString = commandServerPath();
    const QByteArray socketPath = QFile::encodeName(socketPathString);

    auto listenSocket = [&]() -> int {
        const int serverFd = ::socket(AF_UNIX, SOCK_STREAM | SOCK_CLOEXEC, 0);
        if (serverFd == -1) {
            return -1;
        }

        sockaddr_un address {};
        address.sun_family = AF_UNIX;
        std::snprintf(address.sun_path, sizeof(address.sun_path), "%s", socketPath.constData());

        if (::bind(serverFd, reinterpret_cast<const sockaddr *>(&address), sizeof(address)) == -1) {
            ::close(serverFd);
            return -1;
        }

        if (::listen(serverFd, 16) == -1) {
            if (errorMessage) {
                *errorMessage = QString::fromLocal8Bit(std::strerror(errno));
            }
            ::close(serverFd);
            ::unlink(socketPath.constData());
            return -1;
        }

        return serverFd;
    };

    const int firstAttempt = listenSocket();
    if (firstAttempt != -1) {
        return firstAttempt;
    }

    if (errno == EADDRINUSE) {
        QString connectError;
        if (sendCommandToPrimary(QStringLiteral("show-settings"), &connectError)) {
            if (errorMessage) {
                *errorMessage = QStringLiteral("another instance is already running");
            }
            return -1;
        }

        ::unlink(socketPath.constData());
        const int secondAttempt = listenSocket();
        if (secondAttempt != -1) {
            return secondAttempt;
        }
    }

    if (errorMessage && errorMessage->isEmpty()) {
        *errorMessage = QString::fromLocal8Bit(std::strerror(errno));
    }
    return -1;
}

bool isSupportedAction(const QString &action)
{
    return action == QStringLiteral("toggle-overlay")
        || action == QStringLiteral("toggle-crosshair")
        || action == QStringLiteral("toggle-clock")
        || action == QStringLiteral("show-settings")
        || action == QStringLiteral("quit");
}

QAction *registerGlobalAction(
    QObject *parent,
    const QString &id,
    const QString &text,
    const QKeySequence &shortcut,
    QStringList *warnings)
{
    auto *action = new QAction(text, parent);
    action->setObjectName(id);
#ifdef HAVE_KGLOBALACCEL
    const bool defaultOk = KGlobalAccel::self()->setDefaultShortcut(action, {shortcut}, KGlobalAccel::NoAutoloading);
    const bool activeOk = KGlobalAccel::self()->setShortcut(action, {shortcut}, KGlobalAccel::NoAutoloading);
    if ((!defaultOk || !activeOk) && warnings) {
        warnings->append(QStringLiteral("%1 (%2)").arg(text, shortcut.toString()));
    }
#else
    Q_UNUSED(shortcut)
    if (warnings) {
        warnings->append(QStringLiteral("%1 (KGlobalAccel not compiled in)").arg(text));
    }
#endif
    return action;
}

using OverlayList = QVector<QPointer<OverlayWidget>>;

void applyConfigToOverlays(const OverlayList &overlays, const AppConfig &config)
{
    for (OverlayWidget *overlay : overlays) {
        if (overlay) {
            overlay->applyConfig(config);
        }
    }
}

void rebuildOverlays(OverlayList *overlays, const AppConfig &config)
{
    if (!overlays) {
        return;
    }

    for (OverlayWidget *overlay : *overlays) {
        if (overlay) {
            overlay->close();
            overlay->deleteLater();
        }
    }
    overlays->clear();

    const auto screens = QGuiApplication::screens();
    qInfo() << "Rebuilding overlays for" << screens.size() << "screen(s).";
    for (QScreen *screen : screens) {
        qInfo() << "Creating overlay for screen" << screen->name() << screen->geometry();
        auto *overlay = new OverlayWidget(screen);
        overlay->applyConfig(config);
        overlay->configureLayerShell();
        overlay->show();
        overlays->append(overlay);
    }
}

int runDiagnostics(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral(APP_BINARY_NAME));
    QCoreApplication::setOrganizationName(QStringLiteral(APP_ORGANIZATION_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION_STRING));

    QTextStream out(stdout);
    const QString platform = QGuiApplication::platformName();
    out << APP_DISPLAY_NAME << " diagnostics\n";
    out << "Build\n";
    out << "  version=" << QCoreApplication::applicationVersion() << '\n';
    out << "  profile=" << QStringLiteral(APP_BUILD_PROFILE) << '\n';
    out << "  gitCommit=" << QStringLiteral(APP_GIT_COMMIT) << '\n';
    out << "  backendMode=" << backendMode(platform) << '\n';
    out << "Session\n";
    out << "  XDG_SESSION_TYPE=" << qEnvironmentVariable("XDG_SESSION_TYPE") << '\n';
    out << "  XDG_CURRENT_DESKTOP=" << qEnvironmentVariable("XDG_CURRENT_DESKTOP") << '\n';
    out << "  WAYLAND_DISPLAY=" << qEnvironmentVariable("WAYLAND_DISPLAY") << '\n';
    out << "  DISPLAY=" << qEnvironmentVariable("DISPLAY") << '\n';
    out << "Qt\n";
    out << "  platform=" << platform << '\n';
    out << "  pluginsPath=" << QLibraryInfo::path(QLibraryInfo::PluginsPath) << '\n';
    out << "  librariesPath=" << QLibraryInfo::path(QLibraryInfo::LibrariesPath) << '\n';
    out << "  appId=" << QStringLiteral(APP_ID) << '\n';
    out << "  configPath=" << ConfigStore::configPath() << '\n';
    out << "  profilesDir=" << ConfigStore::profilesDir() << '\n';
    out << "  writableConfigLocation=" << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) << '\n';
    out << "  libraryPaths=" << app.libraryPaths().join(QStringLiteral(":")) << '\n';
    out << "Screens\n";
    const auto screens = QGuiApplication::screens();
    out << "  count=" << screens.size() << '\n';
    for (QScreen *screen : screens) {
        out << "  - name=" << screen->name()
            << " geometry=" << screen->geometry().x() << ',' << screen->geometry().y()
            << ' ' << screen->geometry().width() << 'x' << screen->geometry().height()
            << " dpr=" << screen->devicePixelRatio()
            << " refresh=" << screen->refreshRate()
            << '\n';
    }
    out << "Compiled features\n";
#ifdef HAVE_LAYER_SHELL_QT
    out << "  HAVE_LAYER_SHELL_QT=1\n";
#else
    out << "  HAVE_LAYER_SHELL_QT=0\n";
#endif
#ifdef HAVE_KGLOBALACCEL
    out << "  HAVE_KGLOBALACCEL=1\n";
#else
    out << "  HAVE_KGLOBALACCEL=0\n";
#endif
#ifdef HAVE_X11_FALLBACK
    out << "  HAVE_X11_FALLBACK=1\n";
#else
    out << "  HAVE_X11_FALLBACK=0\n";
#endif
    return 0;
}

}

int main(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        const QString arg = QString::fromLocal8Bit(argv[i]);
        if (arg == QStringLiteral("--diagnose") || arg == QStringLiteral("--diagnostics")) {
            return runDiagnostics(argc, argv);
        }
        if (arg == QStringLiteral("--help") || arg == QStringLiteral("-h")) {
            QTextStream out(stdout);
            out << "Usage: " << APP_BINARY_NAME << " [--diagnose] [--version] [action <name>]\n"
                << "Actions: toggle-overlay, toggle-crosshair, toggle-clock, show-settings, quit\n";
            return 0;
        }
        if (arg == QStringLiteral("--version")) {
            QTextStream out(stdout);
            out << APP_DISPLAY_NAME << ' ' << QStringLiteral(APP_VERSION_STRING)
                << " (" << QStringLiteral(APP_GIT_COMMIT)
                << ", profile=" << QStringLiteral(APP_BUILD_PROFILE) << ")\n";
            return 0;
        }
        if (arg == QStringLiteral("action")) {
            QTextStream err(stderr);
            if (i + 1 >= argc) {
                err << "Missing action name.\n";
                return 1;
            }

            const QString action = QString::fromLocal8Bit(argv[++i]);
            if (!isSupportedAction(action)) {
                err << "Unsupported action: " << action << '\n';
                return 1;
            }

            QString errorMessage;
            if (!sendCommandToPrimary(action, &errorMessage)) {
                err << APP_DISPLAY_NAME << " is not running. " << errorMessage << '\n';
                return 1;
            }
            return 0;
        }
    }

    if (sendCommandToPrimary(QStringLiteral("show-settings"))) {
        return 0;
    }

    if (qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland")
#ifdef HAVE_LAYER_SHELL_QT
        && qEnvironmentVariable("XDG_CURRENT_DESKTOP").contains(QStringLiteral("KDE"), Qt::CaseInsensitive)
#else
        && false
#endif
    ) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
    }

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral(APP_BINARY_NAME));
    QCoreApplication::setOrganizationName(QStringLiteral(APP_ORGANIZATION_NAME));
    QCoreApplication::setApplicationVersion(QStringLiteral(APP_VERSION_STRING));
    QApplication::setQuitOnLastWindowClosed(false);

    QString commandServerError;
    const int commandServerFd = createCommandServer(&commandServerError);
    if (commandServerFd == -1) {
        QTextStream err(stderr);
        err << APP_DISPLAY_NAME << " could not start command server: " << commandServerError << '\n';
        return 1;
    }
    QSocketNotifier commandNotifier(commandServerFd, QSocketNotifier::Read);
    QObject::connect(&app, &QCoreApplication::aboutToQuit, &app, [commandServerFd]() {
        ::close(commandServerFd);
        ::unlink(QFile::encodeName(commandServerPath()).constData());
    });

    AppConfig config = ConfigStore::load();

    OverlayList overlays;
    rebuildOverlays(&overlays, config);

    SettingsWindow settings(config);
    QObject::connect(&settings, &SettingsWindow::configChanged, &settings, [&](const AppConfig &updated) {
        config = updated;
        applyConfigToOverlays(overlays, config);
    });
    settings.show();
    QStringList shortcutWarnings;

    QSystemTrayIcon tray;
    const bool trayAvailable = QSystemTrayIcon::isSystemTrayAvailable();
    tray.setIcon(QIcon::fromTheme(QStringLiteral(APP_ID), QIcon::fromTheme(QStringLiteral("applications-graphics"))));
    tray.setToolTip(QStringLiteral(APP_DISPLAY_NAME));

    QMenu trayMenu;
    auto *showAction = trayMenu.addAction(QStringLiteral("Show settings"));
    QObject::connect(showAction, &QAction::triggered, &settings, [&]() {
        settings.show();
        settings.raise();
        settings.activateWindow();
    });
    auto *hideAction = trayMenu.addAction(QStringLiteral("Hide settings"));
    QObject::connect(hideAction, &QAction::triggered, &settings, &QWidget::hide);
    trayMenu.addSeparator();
    auto *quitAction = trayMenu.addAction(QStringLiteral("Quit"));
    QObject::connect(quitAction, &QAction::triggered, &app, &QApplication::quit);

    if (trayAvailable) {
        tray.setContextMenu(&trayMenu);
        QObject::connect(&tray, &QSystemTrayIcon::activated, &settings, [&](QSystemTrayIcon::ActivationReason reason) {
            if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
                settings.isVisible() ? settings.hide() : settings.show();
            }
        });
        tray.show();
    } else {
        qWarning() << "System tray is unavailable; keeping command server active for show/quit actions.";
    }

    auto syncConfig = [&]() {
        ConfigStore::save(config);
        applyConfigToOverlays(overlays, config);
        settings.setConfig(config);
    };

    auto showSettings = [&]() {
        settings.show();
        settings.raise();
        settings.activateWindow();
    };

    auto *toggleOverlay = registerGlobalAction(
        &app,
        QStringLiteral("toggle-overlay"),
        QStringLiteral("Toggle edge overlay"),
        QKeySequence(Qt::Key_F1),
        &shortcutWarnings);
    QObject::connect(toggleOverlay, &QAction::triggered, &app, [&]() {
        config.overlay.visible = !config.overlay.visible;
        syncConfig();
    });

    auto *toggleCrosshair = registerGlobalAction(
        &app,
        QStringLiteral("toggle-crosshair"),
        QStringLiteral("Toggle crosshair"),
        QKeySequence(Qt::Key_F2),
        &shortcutWarnings);
    QObject::connect(toggleCrosshair, &QAction::triggered, &app, [&]() {
        config.crosshair.visible = !config.crosshair.visible;
        syncConfig();
    });

    auto *toggleClock = registerGlobalAction(
        &app,
        QStringLiteral("toggle-clock"),
        QStringLiteral("Toggle clock"),
        QKeySequence(Qt::Key_F3),
        &shortcutWarnings);
    QObject::connect(toggleClock, &QAction::triggered, &app, [&]() {
        config.clock.visible = !config.clock.visible;
        syncConfig();
    });

    auto handleCommand = [&](const QString &command) {
        if (command == QStringLiteral("toggle-overlay")) {
            toggleOverlay->trigger();
            return;
        }
        if (command == QStringLiteral("toggle-crosshair")) {
            toggleCrosshair->trigger();
            return;
        }
        if (command == QStringLiteral("toggle-clock")) {
            toggleClock->trigger();
            return;
        }
        if (command == QStringLiteral("show-settings")) {
            showSettings();
            return;
        }
        if (command == QStringLiteral("quit")) {
            app.quit();
        }
    };

    QObject::connect(&commandNotifier, &QSocketNotifier::activated, &app, [&](int) {
        const int clientFd = ::accept4(commandServerFd, nullptr, nullptr, SOCK_CLOEXEC);
        if (clientFd == -1) {
            return;
        }

        char buffer[256] = {};
        const ssize_t bytesRead = ::read(clientFd, buffer, sizeof(buffer) - 1);
        ::close(clientFd);
        if (bytesRead <= 0) {
            return;
        }

        const QString command = QString::fromUtf8(buffer, static_cast<qsizetype>(bytesRead)).trimmed();
        if (!command.isEmpty()) {
            handleCommand(command);
        }
    });

    QObject::connect(qApp, &QGuiApplication::screenAdded, &settings, [&](QScreen *) {
        rebuildOverlays(&overlays, config);
    });
    QObject::connect(qApp, &QGuiApplication::screenRemoved, &settings, [&](QScreen *) {
        rebuildOverlays(&overlays, config);
    });

    if (!shortcutWarnings.isEmpty()) {
        qWarning().noquote()
            << QStringLiteral("Global shortcuts unavailable:\n%1\n\n"
                              "On KDE, they may already be used by Plasma or another application. "
                              "On non-KDE desktops, KDE global shortcut support may be unavailable.")
                   .arg(shortcutWarnings.join(QStringLiteral("\n")));
    }

    return app.exec();
}
