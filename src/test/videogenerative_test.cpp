#include <gtest/gtest.h>

#include <QColor>
#include <QImage>

#include "video/videogenerative.h"

TEST(VideoGenerativeTest, RenderReturnsSizedOutput) {
    VideoGenerative::Params params;
    params.beatDistance = 0.25;
    params.bpm = 128.0;
    params.beatIndex = 3;
    const QImage out = VideoGenerative::render(params, QSize(640, 360));
    EXPECT_EQ(out.size(), QSize(640, 360));
    EXPECT_FALSE(out.isNull());
}

TEST(VideoGenerativeTest, BeatDistanceChangesOutput) {
    VideoGenerative::Params aParams;
    aParams.beatDistance = 0.0;
    aParams.beatIndex = 1;
    VideoGenerative::Params bParams = aParams;
    bParams.beatDistance = 0.8;

    const QImage a = VideoGenerative::render(aParams, QSize(320, 180));
    const QImage b = VideoGenerative::render(bParams, QSize(320, 180));
    EXPECT_NE(a.pixel(160, 90), b.pixel(160, 90));
}

TEST(VideoGenerativeTest, BeatIndexShiftsBackgroundHue) {
    VideoGenerative::Params paramsA;
    paramsA.beatIndex = 0;
    VideoGenerative::Params paramsB;
    paramsB.beatIndex = 9;

    const QImage a = VideoGenerative::render(paramsA, QSize(64, 64));
    const QImage b = VideoGenerative::render(paramsB, QSize(64, 64));
    EXPECT_NE(a.pixel(8, 8), b.pixel(8, 8));
}

TEST(VideoGenerativeTest, EmptySizeReturnsNull) {
    VideoGenerative::Params params;
    EXPECT_TRUE(VideoGenerative::render(params, QSize()).isNull());
}
