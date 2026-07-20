#include "AppConfig.h"
#include "OverlayWidget.h"
#include "SettingsWindow.h"

#ifdef HAVE_KGLOBALACCEL
#include <KGlobalAccel>
#endif

#include <QAction>
#include <QApplication>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QGuiApplication>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPointer>
#include <QScreen>
#include <QStandardPaths>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QTextStream>
#include <QVector>

namespace {

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
    QCoreApplication::setApplicationName(QStringLiteral("motion-stabilizer-linux"));
    QCoreApplication::setOrganizationName(QStringLiteral("MotionStabilizer"));

    QTextStream out(stdout);
    out << "Motion Stabilizer Linux diagnostics\n";
    out << "Session\n";
    out << "  XDG_SESSION_TYPE=" << qEnvironmentVariable("XDG_SESSION_TYPE") << '\n';
    out << "  XDG_CURRENT_DESKTOP=" << qEnvironmentVariable("XDG_CURRENT_DESKTOP") << '\n';
    out << "  WAYLAND_DISPLAY=" << qEnvironmentVariable("WAYLAND_DISPLAY") << '\n';
    out << "  DISPLAY=" << qEnvironmentVariable("DISPLAY") << '\n';
    out << "Qt\n";
    out << "  platform=" << QGuiApplication::platformName() << '\n';
    out << "  configPath=" << ConfigStore::configPath() << '\n';
    out << "  profilesDir=" << ConfigStore::profilesDir() << '\n';
    out << "  writableConfigLocation=" << QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) << '\n';
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
    out << "Runtime files\n";
    const QList<QPair<QString, QString>> paths = {
        {QStringLiteral("LayerShellQt CMake"), QStringLiteral("/usr/lib/cmake/LayerShellQt/LayerShellQtConfig.cmake")},
        {QStringLiteral("KGlobalAccel CMake"), QStringLiteral("/usr/lib/cmake/KF6GlobalAccel/KF6GlobalAccelConfig.cmake")},
        {QStringLiteral("LayerShellQt plugin"), QStringLiteral("/usr/lib/qt6/plugins/wayland-shell-integration/liblayer-shell.so")},
        {QStringLiteral("LayerShellQt library"), QStringLiteral("/usr/lib/libLayerShellQtInterface.so")},
        {QStringLiteral("KGlobalAccel library"), QStringLiteral("/usr/lib/libKF6GlobalAccel.so")},
    };
    for (const auto &entry : paths) {
        out << "  " << (QFileInfo::exists(entry.second) ? "[ok] " : "[missing] ")
            << entry.first << ": " << entry.second << '\n';
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
            out << "Usage: motion-stabilizer-linux [--diagnose]\n";
            return 0;
        }
    }

    if (qEnvironmentVariable("XDG_SESSION_TYPE") == QStringLiteral("wayland")) {
        qputenv("QT_WAYLAND_SHELL_INTEGRATION", "layer-shell");
    }

    QApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("motion-stabilizer-linux"));
    QCoreApplication::setOrganizationName(QStringLiteral("MotionStabilizer"));

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
    tray.setIcon(QIcon::fromTheme(QStringLiteral("applications-graphics")));
    tray.setToolTip(QStringLiteral("Motion Stabilizer Linux"));

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

    tray.setContextMenu(&trayMenu);
    QObject::connect(&tray, &QSystemTrayIcon::activated, &settings, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::DoubleClick || reason == QSystemTrayIcon::Trigger) {
            settings.isVisible() ? settings.hide() : settings.show();
        }
    });
    tray.show();

    auto syncConfig = [&]() {
        ConfigStore::save(config);
        applyConfigToOverlays(overlays, config);
        settings.setConfig(config);
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

    QObject::connect(qApp, &QGuiApplication::screenAdded, &settings, [&](QScreen *) {
        rebuildOverlays(&overlays, config);
    });
    QObject::connect(qApp, &QGuiApplication::screenRemoved, &settings, [&](QScreen *) {
        rebuildOverlays(&overlays, config);
    });

    if (!shortcutWarnings.isEmpty()) {
        QMessageBox::warning(
            &settings,
            QStringLiteral("Global Shortcut Warning"),
            QStringLiteral("Some global shortcuts could not be registered:\n\n%1\n\n"
                           "On KDE, they may already be used by Plasma or another application. "
                           "On non-KDE desktops, KDE global shortcut support may be unavailable.")
                .arg(shortcutWarnings.join(QStringLiteral("\n"))));
    }

    return app.exec();
}
