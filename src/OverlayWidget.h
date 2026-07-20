#pragma once

#include "AppConfig.h"

#include <QPointer>
#include <QWidget>

class QScreen;

class OverlayWidget final : public QWidget {
    Q_OBJECT

public:
    explicit OverlayWidget(QScreen *screen, QWidget *parent = nullptr);

    void applyConfig(const AppConfig &config);
    void configureLayerShell();
    void configureX11Fallback();
    QScreen *overlayScreen() const;

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    QColor withOpacity(const QColor &color, int opacity) const;
    QRectF safeArea(const QRectF &area, AspectRatio aspectRatio) const;
    QList<QRectF> splitAreas(const QRectF &area, SplitScreen split) const;
    void drawEdgeOverlay(QPainter &painter, const QRectF &area);
    void drawSingleEdgeOverlay(QPainter &painter, const QRectF &area);
    void drawCrosshair(QPainter &painter, const QRectF &area);
    void drawClock(QPainter &painter);

    AppConfig m_config;
    QPointer<QScreen> m_screen;
};
