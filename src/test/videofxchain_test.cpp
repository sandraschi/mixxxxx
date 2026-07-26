#include <gtest/gtest.h>

#include <QColor>
#include <QImage>

#include "control/controlobject.h"
#include "control/controlpushbutton.h"
#include "control/controlpotmeter.h"
#include "test/mixxxtest.h"
#include "video/videofxchain.h"

namespace {

class VideoFxChainTest : public MixxxTest {};

TEST_F(VideoFxChainTest, DivisionFromCoValueMapsSteps) {
    EXPECT_EQ(VideoFxChain::divisionFromCoValue(0.0), 1);
    EXPECT_EQ(VideoFxChain::divisionFromCoValue(2.0), 4);
    EXPECT_EQ(VideoFxChain::divisionFromCoValue(5.0), 32);
    EXPECT_EQ(VideoFxChain::divisionFromCoValue(99.0), 32);
}

TEST_F(VideoFxChainTest, BeatIndexFromGroupUsesClosestBeat) {
    ControlObject bpmCo(ConfigKey("[Channel1]", "bpm"));
    ControlObject closestCo(ConfigKey("[Channel1]", "beat_closest"));
    ControlObject sampleRateCo(ConfigKey("[App]", "samplerate"));

    bpmCo.set(120.0);
    sampleRateCo.set(44100.0);
    closestCo.set(44100.0); // beat 2 at 120 BPM

    EXPECT_EQ(VideoFxChain::beatIndexFromGroup("[Channel1]"), 2);
}

TEST_F(VideoFxChainTest, BeatFxActiveHonorsDivision) {
    EXPECT_TRUE(VideoFxChain::beatFxActive(4, 4));
    EXPECT_FALSE(VideoFxChain::beatFxActive(1, 4));
    EXPECT_TRUE(VideoFxChain::beatFxActive(8, 8));
}

TEST_F(VideoFxChainTest, StrobeStrengthPeaksAtBeatOnset) {
    EXPECT_DOUBLE_EQ(VideoFxChain::strobeStrength(0.0, 120.0), 1.0);
    EXPECT_GT(VideoFxChain::strobeStrength(0.05, 174.0), 0.0);
    EXPECT_DOUBLE_EQ(VideoFxChain::strobeStrength(0.5, 174.0), 0.0);
}

TEST_F(VideoFxChainTest, StrobeWindowWidensAtHighBpm) {
    const double highBpmStrength = VideoFxChain::strobeStrength(0.12, 200.0);
    EXPECT_GT(highBpmStrength, 0.0);
}

TEST_F(VideoFxChainTest, ApplyStrobeBrightensFrame) {
    QImage dark(8, 8, QImage::Format_RGBA8888);
    dark.fill(QColor(10, 10, 10));
    const QImage lit = VideoFxChain::applyStrobe(dark, 1.0);
    EXPECT_GT(qRed(lit.pixel(2, 2)), qRed(dark.pixel(2, 2)));
}

TEST_F(VideoFxChainTest, ApplyZoomPumpKeepsDimensions) {
    QImage src(32, 24, QImage::Format_RGBA8888);
    src.fill(Qt::blue);
    const QImage zoomed = VideoFxChain::applyZoomPump(src, 0.0, 1.0);
    EXPECT_EQ(zoomed.size(), src.size());
}

TEST_F(VideoFxChainTest, ApplyForDeckReadsStrobeCo) {
    ControlPushButton strobeCo(ConfigKey("[Channel1]", "video_beat_fx_strobe"), true, 0.0);
    ControlPushButton zoomCo(ConfigKey("[Channel1]", "video_beat_fx_zoom"), true, 0.0);
    ControlPotmeter divisionCo(ConfigKey("[Channel1]", "video_beat_fx_division"), 0.0, 5.0, true);
    ControlPotmeter strobeAmountCo(
            ConfigKey("[Channel1]", "video_beat_fx_strobe_amount"), 0.0, 1.0, true);
    ControlObject beatDistanceCo(ConfigKey("[Channel1]", "beat_distance"));
    ControlObject bpmCo(ConfigKey("[Channel1]", "bpm"));
    ControlObject closestCo(ConfigKey("[Channel1]", "beat_closest"));
    ControlObject sampleRateCo(ConfigKey("[App]", "samplerate"));

    strobeCo.set(1.0);
    strobeAmountCo.set(1.0);
    divisionCo.set(0.0);
    beatDistanceCo.set(0.02);
    bpmCo.set(120.0);
    sampleRateCo.set(44100.0);
    closestCo.set(0.0);

    QImage dark(16, 16, QImage::Format_RGBA8888);
    dark.fill(QColor(20, 20, 20));
    const QImage out = VideoFxChain::applyForDeck(1, dark);
    EXPECT_GT(qRed(out.pixel(4, 4)), qRed(dark.pixel(4, 4)));

    strobeCo.set(0.0);
}

TEST_F(VideoFxChainTest, ApplyForDeckSkipsWhenDivisionBlocksBeat) {
    ControlPushButton strobeCo(ConfigKey("[Channel1]", "video_beat_fx_strobe"), true, 0.0);
    ControlPushButton zoomCo(ConfigKey("[Channel1]", "video_beat_fx_zoom"), true, 0.0);
    ControlPotmeter divisionCo(ConfigKey("[Channel1]", "video_beat_fx_division"), 0.0, 5.0, true);
    ControlObject beatDistanceCo(ConfigKey("[Channel1]", "beat_distance"));
    ControlObject bpmCo(ConfigKey("[Channel1]", "bpm"));
    ControlObject closestCo(ConfigKey("[Channel1]", "beat_closest"));
    ControlObject sampleRateCo(ConfigKey("[App]", "samplerate"));

    strobeCo.set(1.0);
    divisionCo.set(2.0); // every 4 beats
    beatDistanceCo.set(0.02);
    bpmCo.set(120.0);
    sampleRateCo.set(44100.0);
    closestCo.set(44100.0); // beat index 1, blocked by division 4

    QImage dark(8, 8, QImage::Format_RGBA8888);
    dark.fill(QColor(5, 5, 5));
    const QImage out = VideoFxChain::applyForDeck(1, dark);
    EXPECT_EQ(qRed(out.pixel(2, 2)), qRed(dark.pixel(2, 2)));

    strobeCo.set(0.0);
}

} // namespace
