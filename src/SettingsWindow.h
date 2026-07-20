#pragma once

#include "AppConfig.h"

#include <QMainWindow>

class QCheckBox;
class QComboBox;
class QPushButton;
class QSlider;

class SettingsWindow final : public QMainWindow {
    Q_OBJECT

public:
    explicit SettingsWindow(const AppConfig &config, QWidget *parent = nullptr);

    void setConfig(const AppConfig &config);

signals:
    void configChanged(const AppConfig &config);

private:
    void rebuildUi();
    QWidget *buildOverlayPanel();
    QWidget *buildCrosshairPanel();
    QWidget *buildClockPanel();
    QWidget *buildProfilesPanel();
    QSlider *makeSlider(int min, int max, int value);
    QPushButton *makeColorButton(const QColor &color);
    QString text(const QString &english, const QString &chinese) const;
    void syncUi();
    void refreshProfileList();
    void emitAndSave();
    void chooseOverlayColor();
    void chooseCrosshairColor();
    void saveProfile();
    void loadSelectedProfile();
    void deleteSelectedProfile();

    AppConfig m_config;
    QComboBox *m_language = nullptr;
    QCheckBox *m_overlayVisible = nullptr;
    QComboBox *m_overlayShape = nullptr;
    QComboBox *m_overlayAspectRatio = nullptr;
    QComboBox *m_overlaySplit = nullptr;
    QSlider *m_overlaySize = nullptr;
    QSlider *m_overlayOpacity = nullptr;
    QPushButton *m_overlayColor = nullptr;
    QCheckBox *m_crosshairVisible = nullptr;
    QComboBox *m_crosshairShape = nullptr;
    QComboBox *m_crosshairAspectRatio = nullptr;
    QComboBox *m_crosshairSplit = nullptr;
    QSlider *m_crosshairSize = nullptr;
    QSlider *m_crosshairThickness = nullptr;
    QSlider *m_crosshairOpacity = nullptr;
    QPushButton *m_crosshairColor = nullptr;
    QCheckBox *m_clockVisible = nullptr;
    QCheckBox *m_clockShowSeconds = nullptr;
    QSlider *m_clockFontSize = nullptr;
    QSlider *m_clockOpacity = nullptr;
    QSlider *m_clockX = nullptr;
    QSlider *m_clockY = nullptr;
    QPushButton *m_clockColor = nullptr;
    QComboBox *m_profileList = nullptr;
};
