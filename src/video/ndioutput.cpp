#include "video/ndioutput.h"

#include "control/controlpushbutton.h"
#include "control/controlobject.h"
#include "video/ndi_frame_util.h"
#include "video/videomixer.h"
#include "moc_ndioutput.cpp"

#include <QDebug>

#ifdef __MIXXXXX_NDI__
#include <Processing.NDI.Lib.h>
#endif

namespace mixxx {

constexpr int kOutputFpsNumerator = 30000;
constexpr int kOutputFpsDenominator = 1001;

#ifdef __MIXXXXX_NDI__
int s_ndiLibUsers = 0;

bool acquireNdiLib() {
    if (s_ndiLibUsers == 0 && !NDIlib_initialize()) {
        return false;
    }
    s_ndiLibUsers++;
    return true;
}

void releaseNdiLib() {
    if (s_ndiLibUsers > 0 && --s_ndiLibUsers == 0) {
        NDIlib_destroy();
    }
}

struct NdiOutput::NdiSenderImpl {
    NDIlib_send_instance_t sender = nullptr;
    int64_t frameIndex = 0;
};
#endif

NdiOutput::NdiOutput(UserSettingsPointer pConfig, QObject* parent)
        : QObject(parent),
          m_pConfig(std::move(pConfig)) {
    m_pEnabled = std::make_unique<ControlPushButton>(
            ConfigKey(QStringLiteral("[Ndi]"), QStringLiteral("enabled")), true, 0.0);

    connect(&m_timer, &QTimer::timeout, this, &NdiOutput::slotTick);
    connect(m_pEnabled.get(),
            &ControlPushButton::valueChanged,
            this,
            &NdiOutput::slotEnabledChanged);

    m_timer.setInterval(33);
    if (m_pEnabled->get() > 0.0) {
        startSender();
        m_timer.start();
    }
}

NdiOutput::~NdiOutput() {
    m_timer.stop();
    stopSender();
}

bool NdiOutput::sdkAvailable() const {
#ifdef __MIXXXXX_NDI__
    return true;
#else
    return false;
#endif
}

bool NdiOutput::isSending() const {
#ifdef __MIXXXXX_NDI__
    return m_pSender && m_pSender->sender != nullptr;
#else
    return false;
#endif
}

QString NdiOutput::sourceName() const {
    if (m_pConfig) {
        const QString configured = m_pConfig->getValue<QString>(
                ConfigKey(QStringLiteral("[Ndi]"), QStringLiteral("source_name")));
        if (!configured.trimmed().isEmpty()) {
            return configured.trimmed();
        }
    }
    return QStringLiteral("Mixxxxx");
}

void NdiOutput::slotEnabledChanged(double value) {
    if (value > 0.0) {
        startSender();
        m_timer.start();
        sendBlendedFrame();
        return;
    }
    m_timer.stop();
    stopSender();
}

void NdiOutput::startSender() {
#ifdef __MIXXXXX_NDI__
    if (!m_pSender) {
        m_pSender = std::make_unique<NdiSenderImpl>();
    }
    if (!m_pSender->sender) {
        if (!acquireNdiLib()) {
            qWarning() << "NdiOutput: NDIlib_initialize failed";
            return;
        }

        NDIlib_send_create_t createParams{};
        m_senderNameUtf8 = sourceName().toUtf8();
        createParams.p_ndi_name = m_senderNameUtf8.constData();
        m_pSender->sender = NDIlib_send_create(&createParams);
        if (!m_pSender->sender) {
            qWarning() << "NdiOutput: failed to create sender" << sourceName();
            releaseNdiLib();
            return;
        }
        qInfo() << "NdiOutput: publishing NDI source" << sourceName();
    }
#else
    qInfo() << "NdiOutput: enabled, but build has no NDI SDK (configure with -DNDI=ON)";
#endif
}

void NdiOutput::stopSender() {
#ifdef __MIXXXXX_NDI__
    if (!m_pSender) {
        return;
    }
    if (m_pSender->sender) {
        NDIlib_send_destroy(m_pSender->sender);
        m_pSender->sender = nullptr;
        m_pSender->frameIndex = 0;
        releaseNdiLib();
    }
#endif
}

void NdiOutput::slotTick() {
    if (!m_pEnabled || m_pEnabled->get() <= 0.0) {
        return;
    }
    sendBlendedFrame();
}

void NdiOutput::sendBlendedFrame() {
    ControlObject* co = ControlObject::getControl(
            ConfigKey(QStringLiteral("[Mixer]"), QStringLiteral("crossfader")));
    const double xfader = co ? co->get() : 0.0;
    const QImage blended = VideoMixer::instance().blendFrame(xfader);
    if (blended.isNull()) {
        return;
    }

    const QImage frame = NdiFrameUtil::letterboxToBgra(blended);
    if (frame.isNull()) {
        return;
    }

#ifndef __MIXXXXX_NDI__
    Q_UNUSED(frame);
    return;
#else
    if (!m_pSender || !m_pSender->sender) {
        return;
    }

    NDIlib_video_frame_v2_t videoFrame{};
    videoFrame.xres = frame.width();
    videoFrame.yres = frame.height();
    videoFrame.FourCC = NDIlib_FourCC_type_BGRA;
    videoFrame.frame_rate_N = kOutputFpsNumerator;
    videoFrame.frame_rate_D = kOutputFpsDenominator;
    videoFrame.picture_aspect_ratio = static_cast<float>(frame.width()) / frame.height();
    videoFrame.frame_format_type = NDIlib_frame_format_type_progressive;
    videoFrame.timecode = m_pSender->frameIndex++;
    videoFrame.p_data = const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(frame.constBits()));
    videoFrame.line_stride_in_bytes = frame.bytesPerLine();
    videoFrame.p_metadata = nullptr;

    NDIlib_send_send_video_v2(m_pSender->sender, &videoFrame);
#endif
}

} // namespace mixxx
