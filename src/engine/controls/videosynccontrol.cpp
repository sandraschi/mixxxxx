#include "engine/controls/videosynccontrol.h"

#include "control/controlobject.h"
#include "moc_videosynccontrol.cpp"

VideoSyncControl::VideoSyncControl(const QString& group,
        UserSettingsPointer pConfig)
        : EngineControl(group, pConfig),
          m_pAudioClock(std::make_unique<ControlObject>(
                  ConfigKey(group, "video_audio_clock"), false, false, false, 0.0)) {
}

void VideoSyncControl::process(const double dRate,
        mixxx::audio::FramePos currentPosition,
        const int iBufferSize) {
    Q_UNUSED(dRate);
    Q_UNUSED(iBufferSize);

    auto info = frameInfo();
    if (currentPosition.isValid() && info.sampleRate.isValid()) {
        double clockSeconds = currentPosition.value() / info.sampleRate;
        m_pAudioClock->set(clockSeconds);
    }
}
