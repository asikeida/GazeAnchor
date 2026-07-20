#include "AppConfig.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

QJsonObject colorToJson(const QColor &color)
{
    return {
        {QStringLiteral("r"), color.red()},
        {QStringLiteral("g"), color.green()},
        {QStringLiteral("b"), color.blue()},
    };
}

QColor colorFromJson(const QJsonObject &object, const QColor &fallback)
{
    const int r = object.value(QStringLiteral("r")).toInt(fallback.red());
    const int g = object.value(QStringLiteral("g")).toInt(fallback.green());
    const int b = object.value(QStringLiteral("b")).toInt(fallback.blue());
    return QColor(qBound(0, r, 255), qBound(0, g, 255), qBound(0, b, 255));
}

int boundedInt(const QJsonObject &object, const QString &key, int fallback, int min, int max)
{
    return qBound(min, object.value(key).toInt(fallback), max);
}

OverlayShape overlayShapeFromInt(int value)
{
    if (value < static_cast<int>(OverlayShape::Box) || value > static_cast<int>(OverlayShape::Flag)) {
        return OverlayShape::Box;
    }
    return static_cast<OverlayShape>(value);
}

CrosshairShape crosshairShapeFromInt(int value)
{
    if (value < static_cast<int>(CrosshairShape::Cross) || value > static_cast<int>(CrosshairShape::Diamond)) {
        return CrosshairShape::Cross;
    }
    return static_cast<CrosshairShape>(value);
}

AspectRatio aspectRatioFromInt(int value)
{
    if (value < static_cast<int>(AspectRatio::Ratio16x9) || value > static_cast<int>(AspectRatio::Ratio5x4)) {
        return AspectRatio::Ratio16x9;
    }
    return static_cast<AspectRatio>(value);
}

SplitScreen splitScreenFromInt(int value)
{
    if (value < static_cast<int>(SplitScreen::None) || value > static_cast<int>(SplitScreen::Horizontal)) {
        return SplitScreen::None;
    }
    return static_cast<SplitScreen>(value);
}

UiLanguage languageFromInt(int value)
{
    if (value < static_cast<int>(UiLanguage::English) || value > static_cast<int>(UiLanguage::Chinese)) {
        return UiLanguage::English;
    }
    return static_cast<UiLanguage>(value);
}

QString sanitizedProfileName(QString name)
{
    name = name.trimmed();
    static const QRegularExpression invalid(QStringLiteral(R"([\\/:*?"<>|])"));
    name.replace(invalid, QStringLiteral("_"));
    return name.isEmpty() ? QStringLiteral("Default") : name;
}

QJsonObject configToJson(const AppConfig &config)
{
    QJsonObject ui {
        {QStringLiteral("language"), static_cast<int>(config.ui.language)},
    };

    QJsonObject overlay {
        {QStringLiteral("visible"), config.overlay.visible},
        {QStringLiteral("shape"), static_cast<int>(config.overlay.shape)},
        {QStringLiteral("aspectRatio"), static_cast<int>(config.overlay.aspectRatio)},
        {QStringLiteral("split"), static_cast<int>(config.overlay.split)},
        {QStringLiteral("color"), colorToJson(config.overlay.color)},
        {QStringLiteral("opacity"), config.overlay.opacity},
        {QStringLiteral("size"), config.overlay.size},
    };

    QJsonObject crosshair {
        {QStringLiteral("visible"), config.crosshair.visible},
        {QStringLiteral("shape"), static_cast<int>(config.crosshair.shape)},
        {QStringLiteral("aspectRatio"), static_cast<int>(config.crosshair.aspectRatio)},
        {QStringLiteral("split"), static_cast<int>(config.crosshair.split)},
        {QStringLiteral("color"), colorToJson(config.crosshair.color)},
        {QStringLiteral("opacity"), config.crosshair.opacity},
        {QStringLiteral("size"), config.crosshair.size},
        {QStringLiteral("thickness"), config.crosshair.thickness},
    };

    QJsonObject clock {
        {QStringLiteral("visible"), config.clock.visible},
        {QStringLiteral("color"), colorToJson(config.clock.color)},
        {QStringLiteral("opacity"), config.clock.opacity},
        {QStringLiteral("fontSize"), config.clock.fontSize},
        {QStringLiteral("x"), config.clock.x},
        {QStringLiteral("y"), config.clock.y},
        {QStringLiteral("showSeconds"), config.clock.showSeconds},
    };

    return {
        {QStringLiteral("ui"), ui},
        {QStringLiteral("overlay"), overlay},
        {QStringLiteral("crosshair"), crosshair},
        {QStringLiteral("clock"), clock},
    };
}

AppConfig configFromJson(const QJsonObject &root)
{
    AppConfig config;

    const auto ui = root.value(QStringLiteral("ui")).toObject();
    config.ui.language = languageFromInt(ui.value(QStringLiteral("language")).toInt(static_cast<int>(config.ui.language)));

    const auto overlay = root.value(QStringLiteral("overlay")).toObject();
    config.overlay.visible = overlay.value(QStringLiteral("visible")).toBool(config.overlay.visible);
    config.overlay.shape = overlayShapeFromInt(overlay.value(QStringLiteral("shape")).toInt(static_cast<int>(config.overlay.shape)));
    config.overlay.aspectRatio = aspectRatioFromInt(overlay.value(QStringLiteral("aspectRatio")).toInt(static_cast<int>(config.overlay.aspectRatio)));
    config.overlay.split = splitScreenFromInt(overlay.value(QStringLiteral("split")).toInt(static_cast<int>(config.overlay.split)));
    config.overlay.color = colorFromJson(overlay.value(QStringLiteral("color")).toObject(), config.overlay.color);
    config.overlay.opacity = boundedInt(overlay, QStringLiteral("opacity"), config.overlay.opacity, 0, 100);
    config.overlay.size = boundedInt(overlay, QStringLiteral("size"), config.overlay.size, 4, 140);

    const auto crosshair = root.value(QStringLiteral("crosshair")).toObject();
    config.crosshair.visible = crosshair.value(QStringLiteral("visible")).toBool(config.crosshair.visible);
    config.crosshair.shape = crosshairShapeFromInt(crosshair.value(QStringLiteral("shape")).toInt(static_cast<int>(config.crosshair.shape)));
    config.crosshair.aspectRatio = aspectRatioFromInt(crosshair.value(QStringLiteral("aspectRatio")).toInt(static_cast<int>(config.crosshair.aspectRatio)));
    config.crosshair.split = splitScreenFromInt(crosshair.value(QStringLiteral("split")).toInt(static_cast<int>(config.crosshair.split)));
    config.crosshair.color = colorFromJson(crosshair.value(QStringLiteral("color")).toObject(), config.crosshair.color);
    config.crosshair.opacity = boundedInt(crosshair, QStringLiteral("opacity"), config.crosshair.opacity, 0, 100);
    config.crosshair.size = boundedInt(crosshair, QStringLiteral("size"), config.crosshair.size, 6, 120);
    config.crosshair.thickness = boundedInt(crosshair, QStringLiteral("thickness"), config.crosshair.thickness, 1, 20);

    const auto clock = root.value(QStringLiteral("clock")).toObject();
    config.clock.visible = clock.value(QStringLiteral("visible")).toBool(config.clock.visible);
    config.clock.color = colorFromJson(clock.value(QStringLiteral("color")).toObject(), config.clock.color);
    config.clock.opacity = boundedInt(clock, QStringLiteral("opacity"), config.clock.opacity, 0, 100);
    config.clock.fontSize = boundedInt(clock, QStringLiteral("fontSize"), config.clock.fontSize, 8, 96);
    config.clock.x = boundedInt(clock, QStringLiteral("x"), config.clock.x, 0, 10000);
    config.clock.y = boundedInt(clock, QStringLiteral("y"), config.clock.y, 0, 10000);
    config.clock.showSeconds = clock.value(QStringLiteral("showSeconds")).toBool(config.clock.showSeconds);

    return config;
}

bool saveConfigToPath(const QString &path, const AppConfig &config)
{
    const QFileInfo info(path);
    QDir().mkpath(info.absolutePath());

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        return false;
    }

    file.write(QJsonDocument(configToJson(config)).toJson(QJsonDocument::Indented));
    return true;
}

bool loadConfigFromPath(const QString &path, AppConfig *config)
{
    if (!config) {
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const auto doc = QJsonDocument::fromJson(file.readAll());
    if (!doc.isObject()) {
        return false;
    }

    *config = configFromJson(doc.object());
    return true;
}

}

QString ConfigStore::configPath()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    return QDir(base).filePath(QStringLiteral("config.json"));
}

AppConfig ConfigStore::load()
{
    AppConfig config;
    loadConfigFromPath(configPath(), &config);
    return config;
}

bool ConfigStore::save(const AppConfig &config)
{
    return saveConfigToPath(configPath(), config);
}

QString ConfigStore::profilesDir()
{
    return QDir(QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation)).filePath(QStringLiteral("profiles"));
}

QStringList ConfigStore::listProfiles()
{
    QDir dir(profilesDir());
    const auto files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    QStringList profiles;
    for (const QString &file : files) {
        profiles.append(QFileInfo(file).completeBaseName());
    }
    return profiles;
}

bool ConfigStore::saveProfile(const QString &name, const AppConfig &config)
{
    const QString profile = sanitizedProfileName(name);
    const QString path = QDir(profilesDir()).filePath(profile + QStringLiteral(".json"));
    return saveConfigToPath(path, config);
}

bool ConfigStore::loadProfile(const QString &name, AppConfig *config)
{
    const QString profile = sanitizedProfileName(name);
    const QString path = QDir(profilesDir()).filePath(profile + QStringLiteral(".json"));
    return loadConfigFromPath(path, config);
}

bool ConfigStore::deleteProfile(const QString &name)
{
    const QString profile = sanitizedProfileName(name);
    const QString path = QDir(profilesDir()).filePath(profile + QStringLiteral(".json"));
    return QFile::remove(path);
}
