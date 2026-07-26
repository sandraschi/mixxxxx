#include "video/ndi_frame_util.h"

#include <QPainter>

QImage NdiFrameUtil::letterboxToBgra(const QImage& src, const QSize& outputSize) {
    if (src.isNull() || outputSize.isEmpty()) {
        return QImage();
    }

    QImage canvas(outputSize, QImage::Format_ARGB32);
    canvas.fill(Qt::black);

    const double srcAspect = static_cast<double>(src.width()) / src.height();
    const double outAspect = static_cast<double>(outputSize.width()) / outputSize.height();
    QRect target(0, 0, outputSize.width(), outputSize.height());
    if (srcAspect > outAspect) {
        const int h = static_cast<int>(outputSize.width() / srcAspect);
        target = QRect(0, (outputSize.height() - h) / 2, outputSize.width(), h);
    } else {
        const int w = static_cast<int>(outputSize.height() * srcAspect);
        target = QRect((outputSize.width() - w) / 2, 0, w, outputSize.height());
    }

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.drawImage(target, src);
    painter.end();
    return canvas;
}

QByteArray NdiFrameUtil::bgraBytes(const QImage& bgraImage, int* strideBytes) {
    if (!isValidBgraFrame(bgraImage)) {
        if (strideBytes) {
            *strideBytes = 0;
        }
        return QByteArray();
    }
    if (strideBytes) {
        *strideBytes = bgraImage.bytesPerLine();
    }
    return QByteArray(reinterpret_cast<const char*>(bgraImage.constBits()),
            bgraImage.bytesPerLine() * bgraImage.height());
}

bool NdiFrameUtil::isValidBgraFrame(const QImage& image) {
    return !image.isNull() &&
            image.format() == QImage::Format_ARGB32 &&
            image.width() > 0 &&
            image.height() > 0 &&
            image.bytesPerLine() >= image.width() * 4;
}
