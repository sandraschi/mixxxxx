#pragma once

#include "engine/controls/enginecontrol.h"

class ControlObject;
class ControlProxy;

class PhaseControl : public EngineControl {
    Q_OBJECT
  public:
    PhaseControl(const QString& group, UserSettingsPointer pConfig);
    ~PhaseControl() override = default;

    void process(const double dRate,
            mixxx::audio::FramePos currentPosition,
            const int iBufferSize) override;

  private:
    std::unique_ptr<ControlObject> m_pPhase;
    std::unique_ptr<ControlProxy> m_pBeatDistance;
};
