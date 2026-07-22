#pragma once

#include "engine/controls/enginecontrol.h"

#include <memory>

class ControlObject;

class VideoSyncControl : public EngineControl {
    Q_OBJECT
  public:
    VideoSyncControl(const QString& group, UserSettingsPointer pConfig);
    ~VideoSyncControl() override = default;

    void process(const double dRate,
            mixxx::audio::FramePos currentPosition,
            const int iBufferSize) override;

  private:
    std::unique_ptr<ControlObject> m_pAudioClock;
};
