#include <gtest/gtest.h>

#include <QColor>
#include <QImage>

#include "test/mixxxtest.h"
#include "video/videomixer.h"

namespace {

constexpr int kW = 64;
constexpr int kH = 48;

QImage solid(const QColor& c) {
    QImage img(kW, kH, QImage::Format_RGBA8888);
    img.fill(c);
    return img;
}

int redAt(const QImage& img) {
    return qRed(img.pixel(kW / 2, kH / 2));
}

int blueAt(const QImage& img) {
    return qBlue(img.pixel(kW / 2, kH / 2));
}

class VideoMixerTest : public MixxxTest {
  protected:
    void SetUp() override {
        MixxxTest::SetUp();
        // The mixer is a process-wide singleton, so clear any deck slots a
        // previous test may have left behind.
        for (int deck = 1; deck <= 4; ++deck) {
            VideoMixer::instance().unregisterDecoder(deck);
        }
    }

    void TearDown() override {
        for (int deck = 1; deck <= 4; ++deck) {
            VideoMixer::instance().unregisterDecoder(deck);
        }
        MixxxTest::TearDown();
    }
};

// No registered decks means no output at all.
TEST_F(VideoMixerTest, EmptyMixerReturnsNullImage) {
    EXPECT_TRUE(VideoMixer::instance().blendFrame(0.0).isNull());
}

// Regression test for the deck-keying bug: decoders used to register with the
// FFmpeg container stream index (0 for a typical MP4) rather than the deck
// number, so all decks collapsed into one mixer slot and blending could never
// happen. Two distinct deck keys must survive as two independent slots.
TEST_F(VideoMixerTest, TwoDecksOccupySeparateSlots) {
    VideoMixer::instance().registerDecoder(1, nullptr);
    VideoMixer::instance().registerDecoder(2, nullptr);
    VideoMixer::instance().pushFrame(1, solid(Qt::red), 0.0);
    VideoMixer::instance().pushFrame(2, solid(Qt::blue), 0.0);

    // Hard left is pure deck 1, hard right is pure deck 2. If the two decks had
    // collided on a single key, one of these would return the other's colour.
    const QImage left = VideoMixer::instance().blendFrame(-1.0);
    const QImage right = VideoMixer::instance().blendFrame(1.0);

    ASSERT_FALSE(left.isNull());
    ASSERT_FALSE(right.isNull());
    EXPECT_GT(redAt(left), 200);
    EXPECT_LT(blueAt(left), 55);
    EXPECT_GT(blueAt(right), 200);
    EXPECT_LT(redAt(right), 55);
}

// Regression test for the inverted blend curve: the old code used
// fabs(crossfader), which produced pure deck A at the centre and full deck B at
// BOTH extremes. Deck B's contribution must rise monotonically from left to
// right.
TEST_F(VideoMixerTest, CrossfaderBlendsMonotonically) {
    VideoMixer::instance().registerDecoder(1, nullptr);
    VideoMixer::instance().registerDecoder(2, nullptr);
    VideoMixer::instance().pushFrame(1, solid(Qt::black), 0.0);
    VideoMixer::instance().pushFrame(2, solid(Qt::white), 0.0);

    const double positions[] = {-1.0, -0.5, 0.0, 0.5, 1.0};
    int previous = -1;
    for (double pos : positions) {
        const QImage blended = VideoMixer::instance().blendFrame(pos);
        ASSERT_FALSE(blended.isNull()) << "null frame at crossfader " << pos;
        const int level = redAt(blended);
        EXPECT_GT(level, previous)
                << "deck B contribution did not increase at crossfader " << pos;
        previous = level;
    }

    // Centre should be a genuine mix, not a hard cut to either side.
    const int centre = redAt(VideoMixer::instance().blendFrame(0.0));
    EXPECT_GT(centre, 60);
    EXPECT_LT(centre, 195);
}

// Compositing must not mutate the stored source frame. QImage is implicitly
// shared, so painting into a shallow copy would burn deck B into deck A's
// cached frame and compound on every repaint.
TEST_F(VideoMixerTest, BlendDoesNotMutateStoredFrames) {
    VideoMixer::instance().registerDecoder(1, nullptr);
    VideoMixer::instance().registerDecoder(2, nullptr);
    VideoMixer::instance().pushFrame(1, solid(Qt::black), 0.0);
    VideoMixer::instance().pushFrame(2, solid(Qt::white), 0.0);

    // Repeatedly composite at the centre. If the stored deck 1 frame were being
    // painted into, it would drift towards white with every call.
    int first = -1;
    for (int i = 0; i < 10; ++i) {
        const int level = redAt(VideoMixer::instance().blendFrame(0.0));
        if (first < 0) {
            first = level;
        }
        EXPECT_EQ(level, first) << "blend result drifted on iteration " << i;
    }
}

// A single active deck passes through untouched regardless of crossfader.
TEST_F(VideoMixerTest, SingleDeckPassesThrough) {
    VideoMixer::instance().registerDecoder(1, nullptr);
    VideoMixer::instance().pushFrame(1, solid(Qt::red), 0.0);

    for (double pos : {-1.0, 0.0, 1.0}) {
        const QImage out = VideoMixer::instance().blendFrame(pos);
        ASSERT_FALSE(out.isNull());
        EXPECT_GT(redAt(out), 200) << "at crossfader " << pos;
    }
}

// Unregistering frees the slot, so the remaining deck becomes the sole source.
TEST_F(VideoMixerTest, UnregisterReleasesSlot) {
    VideoMixer::instance().registerDecoder(1, nullptr);
    VideoMixer::instance().registerDecoder(2, nullptr);
    VideoMixer::instance().pushFrame(1, solid(Qt::red), 0.0);
    VideoMixer::instance().pushFrame(2, solid(Qt::blue), 0.0);

    VideoMixer::instance().unregisterDecoder(2);

    // Deck 1 is now alone, so even hard right must show red.
    const QImage out = VideoMixer::instance().blendFrame(1.0);
    ASSERT_FALSE(out.isNull());
    EXPECT_GT(redAt(out), 200);
}

// Saturation at 1.0 is a no-op; 0.0 must collapse to greyscale. This control was
// registered and exposed in the UI but never applied to any frame.
TEST_F(VideoMixerTest, SaturationCollapsesToGrey) {
    const QImage src = solid(Qt::red);

    const QImage unchanged = VideoMixer::applySaturation(src, 1.0);
    EXPECT_GT(qRed(unchanged.pixel(1, 1)), 200);

    const QImage grey = VideoMixer::applySaturation(src, 0.0);
    const QRgb px = grey.pixel(1, 1);
    EXPECT_EQ(qRed(px), qGreen(px));
    EXPECT_EQ(qGreen(px), qBlue(px));
}

} // namespace
