#include "engine/controls/phasecontrol.h"

#include "control/controlobject.h"
#include "control/controlproxy.h"
#include "moc_phasecontrol.cpp"

PhaseControl::PhaseControl(const QString& group,
        UserSettingsPointer pConfig)
        : EngineControl(group, pConfig),
          m_pPhase(std::make_unique<ControlObject>(ConfigKey(group, "phase"))),
          m_pBeatDistance(std::make_unique<ControlProxy>(
                  ConfigKey(group, "beat_distance"), this)) {
}

void PhaseControl::process(const double dRate,
        mixxx::audio::FramePos currentPosition,
        const int iBufferSize) {
    Q_UNUSED(dRate);
    Q_UNUSED(currentPosition);
    Q_UNUSED(iBufferSize);
    double beatDistance = m_pBeatDistance->get();
    // beat_distance is 0.0-1.0, convert to degrees 0-360
    m_pPhase->set(beatDistance * 360.0);
}
