#include "OverlayWidget.h"

#ifdef HAVE_LAYER_SHELL_QT
#include <LayerShellQt/Window>
#endif

#include <QApplication>
#include <QDateTime>
#include <QFont>
#include <QGuiApplication>
#include <QPainter>
#include <QPainterPath>
#include <QScreen>
#include <QDebug>
#include <QTimer>
#include <QWindow>

#ifdef HAVE_X11_FALLBACK
#include <X11/Xlib.h>
#include <X11/extensions/Xfixes.h>
#include <X11/extensions/shape.h>
#endif

OverlayWidget::OverlayWidget(QScreen *screen, QWidget *parent)
    : QWidget(parent)
    , m_screen(screen)
{
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setWindowFlag(Qt::FramelessWindowHint);
    setWindowFlag(Qt::Tool);
    setWindowFlag(Qt::WindowStaysOnTopHint);
    setWindowFlag(Qt::WindowTransparentForInput);
    setFocusPolicy(Qt::NoFocus);

    if (m_screen) {
        setGeometry(m_screen->geometry());
    }

    auto *clockTimer = new QTimer(this);
    clockTimer->setInterval(250);
    connect(clockTimer, &QTimer::timeout, this, [this]() {
        if (m_config.clock.visible) {
            update();
        }
    });
    clockTimer->start();
}

void OverlayWidget::applyConfig(const AppConfig &config)
{
    m_config = config;
    update();
}

void OverlayWidget::configureLayerShell()
{
    if (QGuiApplication::platformName() == QStringLiteral("xcb")) {
        qInfo() << "Configuring X11 overlay fallback for screen"
                << (m_screen ? m_screen->name() : QStringLiteral("<none>"));
        configureX11Fallback();
        return;
    }

    qInfo() << "Configuring Wayland layer-shell overlay for screen"
            << (m_screen ? m_screen->name() : QStringLiteral("<none>"));

#ifdef HAVE_LAYER_SHELL_QT
    createWinId();
    auto *handle = windowHandle();
    if (!handle) {
        return;
    }

    auto *layerWindow = LayerShellQt::Window::get(handle);
    if (!layerWindow) {
        return;
    }

    layerWindow->setScope(QStringLiteral("motion-stabilizer-overlay"));
    layerWindow->setLayer(LayerShellQt::Window::LayerOverlay);
    layerWindow->setKeyboardInteractivity(LayerShellQt::Window::KeyboardInteractivityNone);
    layerWindow->setActivateOnShow(false);
    layerWindow->setExclusiveZone(0);
    layerWindow->setAnchors(LayerShellQt::Window::Anchors(
        LayerShellQt::Window::AnchorTop |
        LayerShellQt::Window::AnchorBottom |
        LayerShellQt::Window::AnchorLeft |
        LayerShellQt::Window::AnchorRight));

    if (m_screen) {
        setGeometry(m_screen->geometry());
        layerWindow->setScreen(m_screen);
        layerWindow->setDesiredSize(m_screen->geometry().size());
    }
#else
    qWarning() << "LayerShellQt was not compiled in. Wayland overlay support is degraded.";
    if (m_screen) {
        setGeometry(m_screen->geometry());
    }
#endif
}

void OverlayWidget::configureX11Fallback()
{
    if (m_screen) {
        setGeometry(m_screen->geometry());
    }

#ifdef HAVE_X11_FALLBACK
    createWinId();
    Display *display = XOpenDisplay(nullptr);
    if (!display) {
        qWarning() << "Could not open X display for X11 input passthrough.";
        return;
    }

    const Window window = static_cast<Window>(winId());
    XserverRegion emptyRegion = XFixesCreateRegion(display, nullptr, 0);
    XFixesSetWindowShapeRegion(display, window, ShapeInput, 0, 0, emptyRegion);
    XFixesDestroyRegion(display, emptyRegion);
    XFlush(display);
    XCloseDisplay(display);
#else
    qWarning() << "X11 fallback was not compiled because x11/xfixes headers or libraries were not found.";
#endif
}

QScreen *OverlayWidget::overlayScreen() const
{
    return m_screen;
}

void OverlayWidget::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);

    const QRectF area = rect();
    if (m_config.overlay.visible) {
        drawEdgeOverlay(painter, area);
    }
    if (m_config.crosshair.visible) {
        drawCrosshair(painter, area);
    }
    if (m_config.clock.visible) {
        drawClock(painter);
    }
}

QColor OverlayWidget::withOpacity(const QColor &color, int opacity) const
{
    QColor out = color;
    out.setAlphaF(qBound(0, opacity, 100) / 100.0);
    return out;
}

QRectF OverlayWidget::safeArea(const QRectF &area, AspectRatio aspectRatio) const
{
    const double screenW = area.width();
    const double screenH = area.height();
    double targetW = screenW;
    double targetH = screenH;

    switch (aspectRatio) {
    case AspectRatio::Ratio21x9:
        targetH = screenW * 9.0 / 21.0;
        if (targetH > screenH) {
            targetH = screenH;
            targetW = screenH * 21.0 / 9.0;
        }
        break;
    case AspectRatio::Ratio4x3:
        targetW = screenH * 4.0 / 3.0;
        if (targetW > screenW) {
            targetW = screenW;
            targetH = screenW * 3.0 / 4.0;
        }
        break;
    case AspectRatio::Ratio5x4:
        targetW = screenH * 5.0 / 4.0;
        if (targetW > screenW) {
            targetW = screenW;
            targetH = screenW * 4.0 / 5.0;
        }
        break;
    case AspectRatio::Ratio16x9:
        break;
    }

    return QRectF(
        area.left() + (screenW - targetW) / 2.0,
        area.top() + (screenH - targetH) / 2.0,
        targetW,
        targetH);
}

QList<QRectF> OverlayWidget::splitAreas(const QRectF &area, SplitScreen split) const
{
    if (split == SplitScreen::Vertical) {
        return {
            QRectF(area.left(), area.top(), area.width() / 2.0, area.height()),
            QRectF(area.center().x(), area.top(), area.width() / 2.0, area.height()),
        };
    }
    if (split == SplitScreen::Horizontal) {
        return {
            QRectF(area.left(), area.top(), area.width(), area.height() / 2.0),
            QRectF(area.left(), area.center().y(), area.width(), area.height() / 2.0),
        };
    }
    return {area};
}

void OverlayWidget::drawEdgeOverlay(QPainter &painter, const QRectF &area)
{
    const QRectF baseArea = safeArea(area, m_config.overlay.aspectRatio);
    for (const QRectF &part : splitAreas(baseArea, m_config.overlay.split)) {
        drawSingleEdgeOverlay(painter, part);
    }
}

void OverlayWidget::drawSingleEdgeOverlay(QPainter &painter, const QRectF &area)
{
    const double thickness = qMax(2, m_config.overlay.size);
    const double length = qMax(thickness * 2.0, thickness * 3.0);
    const double cx = area.center().x();
    const double cy = area.center().y();
    const auto brush = QBrush(withOpacity(m_config.overlay.color, m_config.overlay.opacity));

    painter.setPen(Qt::NoPen);
    painter.setBrush(brush);

    if (m_config.overlay.shape == OverlayShape::Box) {
        painter.drawRect(QRectF(cx - length / 2.0, area.top(), length, thickness));
        painter.drawRect(QRectF(cx - length / 2.0, area.bottom() - thickness, length, thickness));
        painter.drawRect(QRectF(area.left(), cy - length / 2.0, thickness, length));
        painter.drawRect(QRectF(area.right() - thickness, cy - length / 2.0, thickness, length));
        return;
    }

    if (m_config.overlay.shape == OverlayShape::Dome) {
        const double depth = qMax(thickness * 1.5, 8.0);
        const double width = qMax(thickness * 2.5, 36.0);
        painter.drawPie(QRectF(cx - width / 2.0, area.top() - depth, width, depth * 2.0), 180 * 16, 180 * 16);
        painter.drawPie(QRectF(cx - width / 2.0, area.bottom() - depth, width, depth * 2.0), 0, 180 * 16);
        painter.drawPie(QRectF(area.left() - depth, cy - width / 2.0, depth * 2.0, width), 270 * 16, 180 * 16);
        painter.drawPie(QRectF(area.right() - depth, cy - width / 2.0, depth * 2.0, width), 90 * 16, 180 * 16);
        return;
    }

    const double height = qMax(thickness * 4.0, 60.0);
    const double base = qMax(thickness * 2.5, 40.0);
    QPainterPath path;
    path.moveTo(cx - base / 2.0, area.top());
    path.lineTo(cx + base / 2.0, area.top());
    path.lineTo(cx, area.top() + height);
    path.closeSubpath();
    painter.drawPath(path);

    path = QPainterPath();
    path.moveTo(cx - base / 2.0, area.bottom());
    path.lineTo(cx + base / 2.0, area.bottom());
    path.lineTo(cx, area.bottom() - height);
    path.closeSubpath();
    painter.drawPath(path);

    path = QPainterPath();
    path.moveTo(area.left(), cy - base / 2.0);
    path.lineTo(area.left(), cy + base / 2.0);
    path.lineTo(area.left() + height, cy);
    path.closeSubpath();
    painter.drawPath(path);

    path = QPainterPath();
    path.moveTo(area.right(), cy - base / 2.0);
    path.lineTo(area.right(), cy + base / 2.0);
    path.lineTo(area.right() - height, cy);
    path.closeSubpath();
    painter.drawPath(path);
}

void OverlayWidget::drawCrosshair(QPainter &painter, const QRectF &area)
{
    const QRectF baseArea = safeArea(area, m_config.crosshair.aspectRatio);
    const auto parts = splitAreas(baseArea, m_config.crosshair.split);
    const double size = qMax(6, m_config.crosshair.size);
    const double thickness = qMax(1, m_config.crosshair.thickness);
    const QColor color = withOpacity(m_config.crosshair.color, m_config.crosshair.opacity);

    QPen pen(color, thickness, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
    painter.setPen(pen);
    painter.setBrush(Qt::NoBrush);

    for (const QRectF &part : parts) {
        const QPointF center = part.center();
        if (m_config.crosshair.shape == CrosshairShape::Cross) {
            painter.drawLine(QPointF(center.x() - size / 2.0, center.y()), QPointF(center.x() + size / 2.0, center.y()));
            painter.drawLine(QPointF(center.x(), center.y() - size / 2.0), QPointF(center.x(), center.y() + size / 2.0));
            continue;
        }

        if (m_config.crosshair.shape == CrosshairShape::Circle) {
            painter.drawEllipse(center, size / 2.0, size / 2.0);
            continue;
        }

        QPolygonF diamond;
        diamond << QPointF(center.x(), center.y() - size / 2.0)
                << QPointF(center.x() + size / 2.0, center.y())
                << QPointF(center.x(), center.y() + size / 2.0)
                << QPointF(center.x() - size / 2.0, center.y());
        painter.drawPolygon(diamond);
    }
}

void OverlayWidget::drawClock(QPainter &painter)
{
    const QColor color = withOpacity(m_config.clock.color, m_config.clock.opacity);
    const QString format = m_config.clock.showSeconds ? QStringLiteral("HH:mm:ss") : QStringLiteral("HH:mm");
    const QString text = QDateTime::currentDateTime().toString(format);

    QFont font(QStringLiteral("Monospace"));
    font.setStyleHint(QFont::Monospace);
    font.setPointSize(m_config.clock.fontSize);

    painter.setFont(font);
    painter.setPen(QPen(QColor(0, 0, 0, color.alpha()), 4));
    painter.drawText(QPointF(m_config.clock.x + 2, m_config.clock.y + m_config.clock.fontSize), text);
    painter.setPen(color);
    painter.drawText(QPointF(m_config.clock.x, m_config.clock.y + m_config.clock.fontSize), text);
}
