#include "video/videofallback.h"

#include <QPainter>
#include <QtMath>

QImage VideoFallback::renderKenBurns(const QImage& cover,
        double phase01,
        const QSize& outputSize) {
    if (cover.isNull() || outputSize.isEmpty()) {
        return QImage();
    }

    const double phase = phase01 - qFloor(phase01);
    const double zoom = 1.08 + 0.07 * qSin(phase * 2.0 * M_PI);
    const double panX = 0.04 * qSin(phase * M_PI);
    const double panY = 0.03 * qCos(phase * M_PI);

    QImage canvas(outputSize, QImage::Format_RGBA8888);
    canvas.fill(Qt::black);

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    const double canvasAspect = static_cast<double>(outputSize.width()) / outputSize.height();
    const double coverAspect = static_cast<double>(cover.width()) / cover.height();

    QRectF source(0, 0, cover.width(), cover.height());
    if (coverAspect > canvasAspect) {
        const double visibleHeight = cover.height() / zoom;
        const double y = (cover.height() - visibleHeight) * (0.5 + panY);
        source = QRectF(0, y, cover.width(), visibleHeight);
    } else {
        const double visibleWidth = cover.width() / zoom;
        const double x = (cover.width() - visibleWidth) * (0.5 + panX);
        source = QRectF(x, 0, visibleWidth, cover.height());
    }

    painter.drawImage(QRectF(0, 0, outputSize.width(), outputSize.height()),
            cover,
            source);
    painter.end();
    return canvas;
}
