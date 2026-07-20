#pragma once

#include <QColor>
#include <QString>
#include <QStringList>

enum class OverlayShape {
    Box = 0,
    Dome = 1,
    Flag = 2,
};

enum class CrosshairShape {
    Cross = 0,
    Circle = 1,
    Diamond = 2,
};

enum class AspectRatio {
    Ratio16x9 = 0,
    Ratio21x9 = 1,
    Ratio4x3 = 2,
    Ratio5x4 = 3,
};

enum class SplitScreen {
    None = 0,
    Vertical = 1,
    Horizontal = 2,
};

enum class UiLanguage {
    English = 0,
    Chinese = 1,
};

struct UiSettings {
    UiLanguage language = UiLanguage::English;
};

struct OverlaySettings {
    bool visible = true;
    OverlayShape shape = OverlayShape::Box;
    AspectRatio aspectRatio = AspectRatio::Ratio16x9;
    SplitScreen split = SplitScreen::None;
    QColor color = QColor(0, 255, 0);
    int opacity = 60;
    int size = 38;
};

struct CrosshairSettings {
    bool visible = true;
    CrosshairShape shape = CrosshairShape::Cross;
    AspectRatio aspectRatio = AspectRatio::Ratio16x9;
    SplitScreen split = SplitScreen::None;
    QColor color = QColor(255, 0, 0);
    int opacity = 80;
    int size = 24;
    int thickness = 4;
};

struct ClockSettings {
    bool visible = false;
    QColor color = QColor(255, 255, 255);
    int opacity = 100;
    int fontSize = 24;
    int x = 20;
    int y = 20;
    bool showSeconds = false;
};

struct AppConfig {
    UiSettings ui;
    OverlaySettings overlay;
    CrosshairSettings crosshair;
    ClockSettings clock;
};

class ConfigStore {
public:
    static QString configPath();
    static AppConfig load();
    static bool save(const AppConfig &config);
    static QString profilesDir();
    static QStringList listProfiles();
    static bool saveProfile(const QString &name, const AppConfig &config);
    static bool loadProfile(const QString &name, AppConfig *config);
    static bool deleteProfile(const QString &name);
};
