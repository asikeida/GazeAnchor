#include "AppConfig.h"
#include "BuildInfo.h"

#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QTemporaryDir>
#include <QtTest>

class AppConfigTest : public QObject {
    Q_OBJECT

private slots:
    void init();
    void saveWritesSchemaVersionAndLoadsBack();
    void invalidConfigIsBackedUpAndDefaultsAreUsed();
    void profileNamesAreSanitized();

private:
    QTemporaryDir m_tempDir;
};

void AppConfigTest::init()
{
    QVERIFY2(m_tempDir.isValid(), "Temporary config directory must be available");
    qputenv("XDG_CONFIG_HOME", m_tempDir.path().toUtf8());
    QCoreApplication::setApplicationName(QStringLiteral(APP_BINARY_NAME));
    QCoreApplication::setOrganizationName(QStringLiteral(APP_ORGANIZATION_NAME));
}

void AppConfigTest::saveWritesSchemaVersionAndLoadsBack()
{
    AppConfig config;
    config.ui.language = UiLanguage::Chinese;
    config.overlay.visible = false;
    config.overlay.opacity = 42;
    config.crosshair.size = 31;
    config.clock.visible = true;
    config.clock.showSeconds = true;

    QVERIFY(ConfigStore::save(config));

    QFile file(ConfigStore::configPath());
    QVERIFY(file.open(QIODevice::ReadOnly));
    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QVERIFY(doc.isObject());
    QCOMPARE(doc.object().value(QStringLiteral("schemaVersion")).toInt(), 1);

    const AppConfig loaded = ConfigStore::load();
    QCOMPARE(loaded.ui.language, UiLanguage::Chinese);
    QCOMPARE(loaded.overlay.visible, false);
    QCOMPARE(loaded.overlay.opacity, 42);
    QCOMPARE(loaded.crosshair.size, 31);
    QCOMPARE(loaded.clock.visible, true);
    QCOMPARE(loaded.clock.showSeconds, true);
}

void AppConfigTest::invalidConfigIsBackedUpAndDefaultsAreUsed()
{
    QDir().mkpath(QFileInfo(ConfigStore::configPath()).absolutePath());
    QFile file(ConfigStore::configPath());
    QVERIFY(file.open(QIODevice::WriteOnly | QIODevice::Truncate));
    QVERIFY(file.write("{ this is not valid json") > 0);
    file.close();

    const AppConfig loaded = ConfigStore::load();
    QCOMPARE(loaded.ui.language, UiLanguage::English);
    QCOMPARE(loaded.overlay.visible, true);

    QDir dir(QFileInfo(ConfigStore::configPath()).absolutePath());
    const QStringList backups = dir.entryList({QStringLiteral("config.json.corrupt-*")}, QDir::Files, QDir::Name);
    QCOMPARE(backups.size(), 1);
    QVERIFY(!QFileInfo::exists(ConfigStore::configPath()));
}

void AppConfigTest::profileNamesAreSanitized()
{
    AppConfig config;
    config.clock.fontSize = 33;

    QVERIFY(ConfigStore::saveProfile(QStringLiteral("  bad:/name?  "), config));

    const QStringList profiles = ConfigStore::listProfiles();
    QCOMPARE(profiles, QStringList{QStringLiteral("bad__name_")});

    AppConfig loaded;
    QVERIFY(ConfigStore::loadProfile(QStringLiteral("bad:/name?"), &loaded));
    QCOMPARE(loaded.clock.fontSize, 33);
}

QTEST_GUILESS_MAIN(AppConfigTest)

#include "AppConfigTest.moc"
