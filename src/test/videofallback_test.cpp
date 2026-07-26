#include <gtest/gtest.h>

#include <QColor>
#include <QImage>

#include "video/videofallback.h"

TEST(VideoFallbackTest, KenBurnsReturnsSizedOutput) {
    QImage cover(200, 200, QImage::Format_RGBA8888);
    cover.fill(QColor(80, 40, 120));
    const QImage out = VideoFallback::renderKenBurns(cover, 0.25, QSize(640, 360));
    EXPECT_EQ(out.size(), QSize(640, 360));
    EXPECT_FALSE(out.isNull());
}

TEST(VideoFallbackTest, KenBurnsChangesWithPhase) {
    QImage cover(300, 200, QImage::Format_RGBA8888);
    cover.fill(Qt::blue);
    cover.setPixel(150, 100, qRgb(255, 0, 0));

    const QImage a = VideoFallback::renderKenBurns(cover, 0.0, QSize(320, 180));
    const QImage b = VideoFallback::renderKenBurns(cover, 0.5, QSize(320, 180));
    EXPECT_NE(a.pixel(160, 90), b.pixel(160, 90));
}

TEST(VideoFallbackTest, EmptyCoverReturnsNull) {
    QImage empty;
    EXPECT_TRUE(VideoFallback::renderKenBurns(empty, 0.0).isNull());
}
