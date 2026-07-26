#include <gtest/gtest.h>

#include <QColor>
#include <QImage>

#include "video/ndi_frame_util.h"

TEST(NdiFrameUtilTest, LetterboxProducesArgb32) {
    QImage src(400, 200, QImage::Format_RGBA8888);
    src.fill(Qt::red);
    const QImage out = NdiFrameUtil::letterboxToBgra(src, QSize(640, 360));
    EXPECT_EQ(out.size(), QSize(640, 360));
    EXPECT_TRUE(NdiFrameUtil::isValidBgraFrame(out));
}

TEST(NdiFrameUtilTest, BgraBytesMatchImageSize) {
    QImage src(320, 180, QImage::Format_RGBA8888);
    src.fill(Qt::blue);
    const QImage out = NdiFrameUtil::letterboxToBgra(src, QSize(320, 180));
    int stride = 0;
    const QByteArray bytes = NdiFrameUtil::bgraBytes(out, &stride);
    EXPECT_EQ(stride, out.bytesPerLine());
    EXPECT_EQ(bytes.size(), out.bytesPerLine() * out.height());
}

TEST(NdiFrameUtilTest, InvalidImageReturnsEmptyBytes) {
    QImage empty;
    EXPECT_FALSE(NdiFrameUtil::isValidBgraFrame(empty));
    EXPECT_TRUE(NdiFrameUtil::bgraBytes(empty).isEmpty());
}
