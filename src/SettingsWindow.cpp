#include "SettingsWindow.h"

#include <QApplication>
#include <QCheckBox>
#include <QColorDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalBlocker>
#include <QSlider>
#include <QTabWidget>
#include <QVBoxLayout>

SettingsWindow::SettingsWindow(const AppConfig &config, QWidget *parent)
    : QMainWindow(parent)
    , m_config(config)
{
    resize(420, 420);
    rebuildUi();
}

void SettingsWindow::rebuildUi()
{
    setWindowTitle(text(QStringLiteral("GazeAnchor"), QStringLiteral("GazeAnchor")));
    m_language = nullptr;
    m_overlayVisible = nullptr;
    m_overlayShape = nullptr;
    m_overlayAspectRatio = nullptr;
    m_overlaySplit = nullptr;
    m_overlaySize = nullptr;
    m_overlayOpacity = nullptr;
    m_overlayColor = nullptr;
    m_crosshairVisible = nullptr;
    m_crosshairShape = nullptr;
    m_crosshairAspectRatio = nullptr;
    m_crosshairSplit = nullptr;
    m_crosshairSize = nullptr;
    m_crosshairThickness = nullptr;
    m_crosshairOpacity = nullptr;
    m_crosshairColor = nullptr;
    m_clockVisible = nullptr;
    m_clockShowSeconds = nullptr;
    m_clockFontSize = nullptr;
    m_clockOpacity = nullptr;
    m_clockX = nullptr;
    m_clockY = nullptr;
    m_clockColor = nullptr;
    m_profileList = nullptr;

    auto *root = new QWidget(this);
    auto *layout = new QVBoxLayout(root);
    auto *tabs = new QTabWidget(root);
    tabs->addTab(buildOverlayPanel(), text(QStringLiteral("Overlay"), QStringLiteral("边缘叠加")));
    tabs->addTab(buildCrosshairPanel(), text(QStringLiteral("Crosshair"), QStringLiteral("中心准星")));
    tabs->addTab(buildClockPanel(), text(QStringLiteral("Clock"), QStringLiteral("悬浮时钟")));
    tabs->addTab(buildProfilesPanel(), text(QStringLiteral("Profiles"), QStringLiteral("配置方案")));
    layout->addWidget(tabs);

    auto *hint = new QLabel(text(
        QStringLiteral("KDE Wayland build: F1 overlay, F2 crosshair, F3 clock"),
        QStringLiteral("KDE Wayland 版本：F1 开关边缘叠加，F2 开关准星，F3 开关时钟")),
        root);
    hint->setWordWrap(true);
    layout->addWidget(hint);

    setCentralWidget(root);
    syncUi();
    refreshProfileList();
}

void SettingsWindow::setConfig(const AppConfig &config)
{
    m_config = config;
    syncUi();
}

void SettingsWindow::closeEvent(QCloseEvent *event)
{
    QMainWindow::closeEvent(event);
    if (event->isAccepted()) {
        QApplication::quit();
    }
}

QWidget *SettingsWindow::buildOverlayPanel()
{
    auto *box = new QGroupBox(text(QStringLiteral("Edge Overlay"), QStringLiteral("边缘叠加")));
    auto *layout = new QFormLayout(box);

    m_overlayVisible = new QCheckBox(text(QStringLiteral("Visible"), QStringLiteral("显示")), box);
    m_overlayShape = new QComboBox(box);
    m_overlayShape->addItems({
        text(QStringLiteral("Box"), QStringLiteral("方框")),
        text(QStringLiteral("Dome"), QStringLiteral("圆顶")),
        text(QStringLiteral("Flag"), QStringLiteral("旗帜")),
    });
    m_overlayAspectRatio = new QComboBox(box);
    m_overlayAspectRatio->addItems({QStringLiteral("16:9"), QStringLiteral("21:9"), QStringLiteral("4:3"), QStringLiteral("5:4")});
    m_overlaySplit = new QComboBox(box);
    m_overlaySplit->addItems({
        text(QStringLiteral("None"), QStringLiteral("无")),
        text(QStringLiteral("Vertical"), QStringLiteral("垂直")),
        text(QStringLiteral("Horizontal"), QStringLiteral("水平")),
    });
    m_overlaySize = makeSlider(4, 140, m_config.overlay.size);
    m_overlayOpacity = makeSlider(0, 100, m_config.overlay.opacity);
    m_overlayColor = makeColorButton(m_config.overlay.color);

    layout->addRow(m_overlayVisible);
    layout->addRow(text(QStringLiteral("Shape"), QStringLiteral("形状")), m_overlayShape);
    layout->addRow(text(QStringLiteral("Aspect ratio"), QStringLiteral("宽高比")), m_overlayAspectRatio);
    layout->addRow(text(QStringLiteral("Split"), QStringLiteral("分屏")), m_overlaySplit);
    layout->addRow(text(QStringLiteral("Size"), QStringLiteral("大小")), m_overlaySize);
    layout->addRow(text(QStringLiteral("Opacity"), QStringLiteral("透明度")), m_overlayOpacity);
    layout->addRow(text(QStringLiteral("Color"), QStringLiteral("颜色")), m_overlayColor);

    connect(m_overlayVisible, &QCheckBox::toggled, this, [this](bool checked) {
        m_config.overlay.visible = checked;
        emitAndSave();
    });
    connect(m_overlayShape, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.overlay.shape = static_cast<OverlayShape>(index);
        emitAndSave();
    });
    connect(m_overlayAspectRatio, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.overlay.aspectRatio = static_cast<AspectRatio>(index);
        emitAndSave();
    });
    connect(m_overlaySplit, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.overlay.split = static_cast<SplitScreen>(index);
        emitAndSave();
    });
    connect(m_overlaySize, &QSlider::valueChanged, this, [this](int value) {
        m_config.overlay.size = value;
        emitAndSave();
    });
    connect(m_overlayOpacity, &QSlider::valueChanged, this, [this](int value) {
        m_config.overlay.opacity = value;
        emitAndSave();
    });
    connect(m_overlayColor, &QPushButton::clicked, this, &SettingsWindow::chooseOverlayColor);

    return box;
}

QWidget *SettingsWindow::buildCrosshairPanel()
{
    auto *box = new QGroupBox(text(QStringLiteral("Crosshair"), QStringLiteral("中心准星")));
    auto *layout = new QFormLayout(box);

    m_crosshairVisible = new QCheckBox(text(QStringLiteral("Visible"), QStringLiteral("显示")), box);
    m_crosshairShape = new QComboBox(box);
    m_crosshairShape->addItems({
        text(QStringLiteral("Cross"), QStringLiteral("十字")),
        text(QStringLiteral("Circle"), QStringLiteral("圆形")),
        text(QStringLiteral("Diamond"), QStringLiteral("菱形")),
    });
    m_crosshairAspectRatio = new QComboBox(box);
    m_crosshairAspectRatio->addItems({QStringLiteral("16:9"), QStringLiteral("21:9"), QStringLiteral("4:3"), QStringLiteral("5:4")});
    m_crosshairSplit = new QComboBox(box);
    m_crosshairSplit->addItems({
        text(QStringLiteral("None"), QStringLiteral("无")),
        text(QStringLiteral("Vertical"), QStringLiteral("垂直")),
        text(QStringLiteral("Horizontal"), QStringLiteral("水平")),
    });
    m_crosshairSize = makeSlider(6, 120, m_config.crosshair.size);
    m_crosshairThickness = makeSlider(1, 20, m_config.crosshair.thickness);
    m_crosshairOpacity = makeSlider(0, 100, m_config.crosshair.opacity);
    m_crosshairColor = makeColorButton(m_config.crosshair.color);

    layout->addRow(m_crosshairVisible);
    layout->addRow(text(QStringLiteral("Shape"), QStringLiteral("形状")), m_crosshairShape);
    layout->addRow(text(QStringLiteral("Aspect ratio"), QStringLiteral("宽高比")), m_crosshairAspectRatio);
    layout->addRow(text(QStringLiteral("Split"), QStringLiteral("分屏")), m_crosshairSplit);
    layout->addRow(text(QStringLiteral("Size"), QStringLiteral("大小")), m_crosshairSize);
    layout->addRow(text(QStringLiteral("Thickness"), QStringLiteral("线宽")), m_crosshairThickness);
    layout->addRow(text(QStringLiteral("Opacity"), QStringLiteral("透明度")), m_crosshairOpacity);
    layout->addRow(text(QStringLiteral("Color"), QStringLiteral("颜色")), m_crosshairColor);

    connect(m_crosshairVisible, &QCheckBox::toggled, this, [this](bool checked) {
        m_config.crosshair.visible = checked;
        emitAndSave();
    });
    connect(m_crosshairShape, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.crosshair.shape = static_cast<CrosshairShape>(index);
        emitAndSave();
    });
    connect(m_crosshairAspectRatio, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.crosshair.aspectRatio = static_cast<AspectRatio>(index);
        emitAndSave();
    });
    connect(m_crosshairSplit, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.crosshair.split = static_cast<SplitScreen>(index);
        emitAndSave();
    });
    connect(m_crosshairSize, &QSlider::valueChanged, this, [this](int value) {
        m_config.crosshair.size = value;
        emitAndSave();
    });
    connect(m_crosshairThickness, &QSlider::valueChanged, this, [this](int value) {
        m_config.crosshair.thickness = value;
        emitAndSave();
    });
    connect(m_crosshairOpacity, &QSlider::valueChanged, this, [this](int value) {
        m_config.crosshair.opacity = value;
        emitAndSave();
    });
    connect(m_crosshairColor, &QPushButton::clicked, this, &SettingsWindow::chooseCrosshairColor);

    return box;
}

QWidget *SettingsWindow::buildClockPanel()
{
    auto *box = new QGroupBox(text(QStringLiteral("Clock"), QStringLiteral("悬浮时钟")));
    auto *layout = new QFormLayout(box);

    m_clockVisible = new QCheckBox(text(QStringLiteral("Visible"), QStringLiteral("显示")), box);
    m_clockShowSeconds = new QCheckBox(text(QStringLiteral("Show seconds"), QStringLiteral("显示秒")), box);
    m_clockFontSize = makeSlider(8, 96, m_config.clock.fontSize);
    m_clockOpacity = makeSlider(0, 100, m_config.clock.opacity);
    m_clockX = makeSlider(0, 3840, m_config.clock.x);
    m_clockY = makeSlider(0, 2160, m_config.clock.y);
    m_clockColor = makeColorButton(m_config.clock.color);

    layout->addRow(m_clockVisible);
    layout->addRow(m_clockShowSeconds);
    layout->addRow(text(QStringLiteral("Font size"), QStringLiteral("字号")), m_clockFontSize);
    layout->addRow(text(QStringLiteral("Opacity"), QStringLiteral("透明度")), m_clockOpacity);
    layout->addRow(QStringLiteral("X"), m_clockX);
    layout->addRow(QStringLiteral("Y"), m_clockY);
    layout->addRow(text(QStringLiteral("Color"), QStringLiteral("颜色")), m_clockColor);

    connect(m_clockVisible, &QCheckBox::toggled, this, [this](bool checked) {
        m_config.clock.visible = checked;
        emitAndSave();
    });
    connect(m_clockShowSeconds, &QCheckBox::toggled, this, [this](bool checked) {
        m_config.clock.showSeconds = checked;
        emitAndSave();
    });
    connect(m_clockFontSize, &QSlider::valueChanged, this, [this](int value) {
        m_config.clock.fontSize = value;
        emitAndSave();
    });
    connect(m_clockOpacity, &QSlider::valueChanged, this, [this](int value) {
        m_config.clock.opacity = value;
        emitAndSave();
    });
    connect(m_clockX, &QSlider::valueChanged, this, [this](int value) {
        m_config.clock.x = value;
        emitAndSave();
    });
    connect(m_clockY, &QSlider::valueChanged, this, [this](int value) {
        m_config.clock.y = value;
        emitAndSave();
    });
    connect(m_clockColor, &QPushButton::clicked, this, [this]() {
        const QColor color = QColorDialog::getColor(m_config.clock.color, this, QStringLiteral("Clock Color"));
        if (!color.isValid()) {
            return;
        }
        m_config.clock.color = color;
        syncUi();
        emitAndSave();
    });

    return box;
}

QWidget *SettingsWindow::buildProfilesPanel()
{
    auto *panel = new QWidget;
    auto *layout = new QVBoxLayout(panel);

    auto *box = new QGroupBox(text(QStringLiteral("Profiles"), QStringLiteral("配置方案")), panel);
    auto *form = new QFormLayout(box);
    m_language = new QComboBox(box);
    m_language->addItems({QStringLiteral("English"), QStringLiteral("中文")});
    form->addRow(text(QStringLiteral("Language"), QStringLiteral("语言")), m_language);

    m_profileList = new QComboBox(box);
    form->addRow(text(QStringLiteral("Saved profiles"), QStringLiteral("已保存方案")), m_profileList);

    auto *buttons = new QWidget(box);
    auto *buttonLayout = new QHBoxLayout(buttons);
    buttonLayout->setContentsMargins(0, 0, 0, 0);
    auto *save = new QPushButton(text(QStringLiteral("Save current"), QStringLiteral("保存当前")), buttons);
    auto *load = new QPushButton(text(QStringLiteral("Load"), QStringLiteral("加载")), buttons);
    auto *remove = new QPushButton(text(QStringLiteral("Delete"), QStringLiteral("删除")), buttons);
    buttonLayout->addWidget(save);
    buttonLayout->addWidget(load);
    buttonLayout->addWidget(remove);
    form->addRow(buttons);

    connect(save, &QPushButton::clicked, this, &SettingsWindow::saveProfile);
    connect(load, &QPushButton::clicked, this, &SettingsWindow::loadSelectedProfile);
    connect(remove, &QPushButton::clicked, this, &SettingsWindow::deleteSelectedProfile);
    connect(m_language, &QComboBox::currentIndexChanged, this, [this](int index) {
        m_config.ui.language = static_cast<UiLanguage>(index);
        emitAndSave();
        rebuildUi();
    });

    layout->addWidget(box);
    layout->addStretch(1);
    return panel;
}

QSlider *SettingsWindow::makeSlider(int min, int max, int value)
{
    auto *slider = new QSlider(Qt::Horizontal);
    slider->setRange(min, max);
    slider->setValue(value);
    return slider;
}

QPushButton *SettingsWindow::makeColorButton(const QColor &color)
{
    auto *button = new QPushButton(color.name(QColor::HexRgb));
    button->setStyleSheet(QStringLiteral("background-color: %1;").arg(color.name(QColor::HexRgb)));
    return button;
}

QString SettingsWindow::text(const QString &english, const QString &chinese) const
{
    return m_config.ui.language == UiLanguage::Chinese ? chinese : english;
}

void SettingsWindow::syncUi()
{
    const QSignalBlocker blockLanguage(m_language);
    const QSignalBlocker blockOverlayVisible(m_overlayVisible);
    const QSignalBlocker blockOverlayShape(m_overlayShape);
    const QSignalBlocker blockOverlayAspectRatio(m_overlayAspectRatio);
    const QSignalBlocker blockOverlaySplit(m_overlaySplit);
    const QSignalBlocker blockOverlaySize(m_overlaySize);
    const QSignalBlocker blockOverlayOpacity(m_overlayOpacity);
    const QSignalBlocker blockCrosshairVisible(m_crosshairVisible);
    const QSignalBlocker blockCrosshairShape(m_crosshairShape);
    const QSignalBlocker blockCrosshairAspectRatio(m_crosshairAspectRatio);
    const QSignalBlocker blockCrosshairSplit(m_crosshairSplit);
    const QSignalBlocker blockCrosshairSize(m_crosshairSize);
    const QSignalBlocker blockCrosshairThickness(m_crosshairThickness);
    const QSignalBlocker blockCrosshairOpacity(m_crosshairOpacity);
    const QSignalBlocker blockClockVisible(m_clockVisible);
    const QSignalBlocker blockClockShowSeconds(m_clockShowSeconds);
    const QSignalBlocker blockClockFontSize(m_clockFontSize);
    const QSignalBlocker blockClockOpacity(m_clockOpacity);
    const QSignalBlocker blockClockX(m_clockX);
    const QSignalBlocker blockClockY(m_clockY);

    if (m_language) {
        m_language->setCurrentIndex(static_cast<int>(m_config.ui.language));
    }

    m_overlayVisible->setChecked(m_config.overlay.visible);
    m_overlayShape->setCurrentIndex(static_cast<int>(m_config.overlay.shape));
    m_overlayAspectRatio->setCurrentIndex(static_cast<int>(m_config.overlay.aspectRatio));
    m_overlaySplit->setCurrentIndex(static_cast<int>(m_config.overlay.split));
    m_overlaySize->setValue(m_config.overlay.size);
    m_overlayOpacity->setValue(m_config.overlay.opacity);
    m_overlayColor->setText(m_config.overlay.color.name(QColor::HexRgb));
    m_overlayColor->setStyleSheet(QStringLiteral("background-color: %1;").arg(m_config.overlay.color.name(QColor::HexRgb)));

    m_crosshairVisible->setChecked(m_config.crosshair.visible);
    m_crosshairShape->setCurrentIndex(static_cast<int>(m_config.crosshair.shape));
    m_crosshairAspectRatio->setCurrentIndex(static_cast<int>(m_config.crosshair.aspectRatio));
    m_crosshairSplit->setCurrentIndex(static_cast<int>(m_config.crosshair.split));
    m_crosshairSize->setValue(m_config.crosshair.size);
    m_crosshairThickness->setValue(m_config.crosshair.thickness);
    m_crosshairOpacity->setValue(m_config.crosshair.opacity);
    m_crosshairColor->setText(m_config.crosshair.color.name(QColor::HexRgb));
    m_crosshairColor->setStyleSheet(QStringLiteral("background-color: %1;").arg(m_config.crosshair.color.name(QColor::HexRgb)));

    m_clockVisible->setChecked(m_config.clock.visible);
    m_clockShowSeconds->setChecked(m_config.clock.showSeconds);
    m_clockFontSize->setValue(m_config.clock.fontSize);
    m_clockOpacity->setValue(m_config.clock.opacity);
    m_clockX->setValue(m_config.clock.x);
    m_clockY->setValue(m_config.clock.y);
    m_clockColor->setText(m_config.clock.color.name(QColor::HexRgb));
    m_clockColor->setStyleSheet(QStringLiteral("background-color: %1;").arg(m_config.clock.color.name(QColor::HexRgb)));
}

void SettingsWindow::refreshProfileList()
{
    if (!m_profileList) {
        return;
    }

    const QSignalBlocker block(m_profileList);
    const QString current = m_profileList->currentText();
    m_profileList->clear();
    m_profileList->addItems(ConfigStore::listProfiles());
    const int index = m_profileList->findText(current);
    if (index >= 0) {
        m_profileList->setCurrentIndex(index);
    }
}

void SettingsWindow::emitAndSave()
{
    ConfigStore::save(m_config);
    emit configChanged(m_config);
}

void SettingsWindow::saveProfile()
{
    bool ok = false;
    const QString name = QInputDialog::getText(
        this,
        text(QStringLiteral("Save Profile"), QStringLiteral("保存配置方案")),
        text(QStringLiteral("Profile name"), QStringLiteral("方案名称")),
        QLineEdit::Normal,
        QStringLiteral("Default"),
        &ok);
    if (!ok || name.trimmed().isEmpty()) {
        return;
    }

    if (!ConfigStore::saveProfile(name, m_config)) {
        QMessageBox::warning(
            this,
            text(QStringLiteral("Save Profile"), QStringLiteral("保存配置方案")),
            text(QStringLiteral("Could not save the profile."), QStringLiteral("无法保存该配置方案。")));
        return;
    }
    refreshProfileList();
    const int index = m_profileList->findText(name.trimmed());
    if (index >= 0) {
        m_profileList->setCurrentIndex(index);
    }
}

void SettingsWindow::loadSelectedProfile()
{
    if (!m_profileList || m_profileList->currentText().isEmpty()) {
        return;
    }

    AppConfig loaded;
    if (!ConfigStore::loadProfile(m_profileList->currentText(), &loaded)) {
        QMessageBox::warning(
            this,
            text(QStringLiteral("Load Profile"), QStringLiteral("加载配置方案")),
            text(QStringLiteral("Could not load the selected profile."), QStringLiteral("无法加载所选配置方案。")));
        return;
    }

    m_config = loaded;
    syncUi();
    emitAndSave();
}

void SettingsWindow::deleteSelectedProfile()
{
    if (!m_profileList || m_profileList->currentText().isEmpty()) {
        return;
    }

    const QString name = m_profileList->currentText();
    const auto answer = QMessageBox::question(
        this,
        text(QStringLiteral("Delete Profile"), QStringLiteral("删除配置方案")),
        text(QStringLiteral("Delete profile \"%1\"?"), QStringLiteral("删除配置方案 \"%1\"?")).arg(name));
    if (answer != QMessageBox::Yes) {
        return;
    }

    if (!ConfigStore::deleteProfile(name)) {
        QMessageBox::warning(
            this,
            text(QStringLiteral("Delete Profile"), QStringLiteral("删除配置方案")),
            text(QStringLiteral("Could not delete the selected profile."), QStringLiteral("无法删除所选配置方案。")));
        return;
    }
    refreshProfileList();
}

void SettingsWindow::chooseOverlayColor()
{
    const QColor color = QColorDialog::getColor(
        m_config.overlay.color,
        this,
        text(QStringLiteral("Overlay Color"), QStringLiteral("边缘叠加颜色")));
    if (!color.isValid()) {
        return;
    }
    m_config.overlay.color = color;
    syncUi();
    emitAndSave();
}

void SettingsWindow::chooseCrosshairColor()
{
    const QColor color = QColorDialog::getColor(
        m_config.crosshair.color,
        this,
        text(QStringLiteral("Crosshair Color"), QStringLiteral("准星颜色")));
    if (!color.isValid()) {
        return;
    }
    m_config.crosshair.color = color;
    syncUi();
    emitAndSave();
}
