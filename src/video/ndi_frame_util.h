#pragma once

#include <QByteArray>
#include <QImage>
#include <QSize>

// QImage → NDI-compatible BGRA layout (Format_ARGB32 on little-endian x86).
// Testable without the NDI SDK installed.
class NdiFrameUtil {
  public:
    static constexpr int kDefaultWidth = 1280;
    static constexpr int kDefaultHeight = 720;

    static QImage letterboxToBgra(const QImage& src,
            const QSize& outputSize = QSize(kDefaultWidth, kDefaultHeight));

    static QByteArray bgraBytes(const QImage& bgraImage, int* strideBytes = nullptr);

    static bool isValidBgraFrame(const QImage& image);
};
