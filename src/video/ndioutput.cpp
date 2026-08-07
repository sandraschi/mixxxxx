#include "video/ndioutput.h"

#include "control/controlpushbutton.h"
#include "control/controlobject.h"
#include "video/ndi_frame_util.h"
#include "video/ndi_runtime.h"
#include "video/videomixer.h"
#include "moc_ndioutput.cpp"

#include <QAbstractButton>
#include <QDebug>
#include <QDesktopServices>
#include <QMessageBox>
#include <QPushButton>
#include <QUrl>
#include <QWidget>

#ifdef __MIXXXXX_NDI__
#include "mixxxxx_ndi_headers.h"
#endif

namespace mixxx {

constexpr int kOutputFpsNumerator = 30000;
constexpr int kOutputFpsDenominator = 1001;
constexpr const char* kNdiRedistUrl = "https://ndi.link/NDIRedistV5";

#ifdef __MIXXXXX_NDI__

struct NdiOutput::NdiSenderImpl {
    NDIlib_send_instance_t sender = nullptr;
};

NdiSenderWorker::NdiSenderWorker(QObject* parent)
        : QThread(parent) {
}

NdiSenderWorker::~NdiSenderWorker() {
    requestStop();
    wait(3000);
}

void NdiSenderWorker::setSender(void* senderInstance) {
    QMutexLocker lock(&m_mutex);
    m_sender = senderInstance;
}

void NdiSenderWorker::enqueueFrame(NdiPendingFrame frame) {
    QMutexLocker lock(&m_mutex);
    while (static_cast<int>(m_queue.size()) >= kMaxQueueDepth) {
        m_queue.pop_front();
    }
    m_queue.push_back(std::move(frame));
    m_cond.wakeOne();
}

void NdiSenderWorker::requestStop() {
    {
        QMutexLocker lock(&m_mutex);
        m_stop = true;
        m_queue.clear();
    }
    m_cond.wakeAll();
}

void NdiSenderWorker::run() {
    const NDIlib_v5* ndi = NdiRuntime::instance().api();
    if (ndi == nullptr) {
        return;
    }

    while (true) {
        NdiPendingFrame frame;
        void* sender = nullptr;
        {
            QMutexLocker lock(&m_mutex);
            while (m_queue.empty() && !m_stop) {
                m_cond.wait(&m_mutex);
            }
            if (m_stop && m_queue.empty()) {
                break;
            }
            frame = std::move(m_queue.front());
            m_queue.pop_front();
            sender = m_sender;
        }

        if (sender == nullptr || frame.pixels.isEmpty()) {
            continue;
        }

        NDIlib_video_frame_v2_t videoFrame{};
        videoFrame.xres = frame.width;
        videoFrame.yres = frame.height;
        videoFrame.FourCC = NDIlib_FourCC_type_BGRA;
        videoFrame.frame_rate_N = kOutputFpsNumerator;
        videoFrame.frame_rate_D = kOutputFpsDenominator;
        videoFrame.picture_aspect_ratio =
                static_cast<float>(frame.width) / static_cast<float>(frame.height);
        videoFrame.frame_format_type = NDIlib_frame_format_type_progressive;
        videoFrame.timecode = frame.timecode;
        videoFrame.p_data = reinterpret_cast<uint8_t*>(frame.pixels.data());
        videoFrame.line_stride_in_bytes = frame.strideBytes;
        videoFrame.p_metadata = nullptr;

        ndi->send_send_video_v2(static_cast<NDIlib_send_instance_t>(sender), &videoFrame);
    }
}

#endif // __MIXXXXX_NDI__

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
        if (isSending()) {
            m_timer.start();
        }
    }
}

NdiOutput::~NdiOutput() {
    m_timer.stop();
    stopSender();
}

bool NdiOutput::featureCompiled() const {
#ifdef __MIXXXXX_NDI__
    return true;
#else
    return false;
#endif
}

bool NdiOutput::runtimeAvailable() const {
#ifdef __MIXXXXX_NDI__
    return NdiRuntime::instance().isLoaded() || NdiRuntime::instance().tryLoad();
#else
    return false;
#endif
}

bool NdiOutput::isSending() const {
#ifdef __MIXXXXX_NDI__
    return m_pSender && m_pSender->sender != nullptr && m_pWorker && m_pWorker->isRunning();
#else
    return false;
#endif
}

QString NdiOutput::runtimeStatusMessage() const {
#ifdef __MIXXXXX_NDI__
    if (NdiRuntime::instance().isLoaded()) {
        return QString();
    }
    NdiRuntime::instance().tryLoad();
    return NdiRuntime::instance().lastError();
#else
    return QStringLiteral("NDI support was not compiled in (build with -DNDI=ON).");
#endif
}

void NdiOutput::notifyRuntimeMissingOnce() {
    if (m_runtimeWarned) {
        return;
    }
    m_runtimeWarned = true;

    const QString detail = runtimeStatusMessage();
    qWarning() << "NdiOutput:" << detail;

    QWidget* parentWidget = qobject_cast<QWidget*>(parent());
    QMessageBox box(parentWidget);
    box.setIcon(QMessageBox::Warning);
    box.setWindowTitle(tr("NDI® runtime required"));
    box.setText(tr("Mixxxxx could not load the NDI® runtime library."));
    box.setInformativeText(
            tr("%1\n\nInstall the free NDI redistributable, set NDI_RUNTIME_DIR_V5 to its "
               "folder, then restart Mixxxxx.")
                    .arg(detail));
    box.setStandardButtons(QMessageBox::Ok);
    box.setDefaultButton(QMessageBox::Ok);
    box.setEscapeButton(QMessageBox::Ok);
    box.setTextFormat(Qt::PlainText);

    const QString link = QString::fromLatin1(kNdiRedistUrl);
    QPushButton* pDownload =
            box.addButton(tr("Download NDI® runtime…"), QMessageBox::ActionRole);
    box.exec();
    if (box.clickedButton() == pDownload) {
        QDesktopServices::openUrl(QUrl(link));
    }
}

void NdiOutput::slotEnabledChanged(double value) {
    if (value > 0.0) {
        startSender();
        if (isSending()) {
            m_timer.start();
            captureAndQueueFrame();
        }
        return;
    }
    m_timer.stop();
    stopSender();
}

void NdiOutput::startSender() {
#ifdef __MIXXXXX_NDI__
    if (!NdiRuntime::instance().acquire()) {
        notifyRuntimeMissingOnce();
        return;
    }

    if (!m_pSender) {
        m_pSender = std::make_unique<NdiSenderImpl>();
    }
    if (m_pSender->sender != nullptr) {
        return;
    }

    const NDIlib_v5* ndi = NdiRuntime::instance().api();
    if (ndi == nullptr) {
        notifyRuntimeMissingOnce();
        NdiRuntime::instance().release();
        return;
    }

    NDIlib_send_create_t createParams{};
    m_senderNameUtf8 = sourceName().toUtf8();
    createParams.p_ndi_name = m_senderNameUtf8.constData();
    m_pSender->sender = ndi->send_create(&createParams);
    if (!m_pSender->sender) {
        qWarning() << "NdiOutput: failed to create NDI® sender" << sourceName();
        NdiRuntime::instance().release();
        return;
    }

    if (!m_pWorker) {
        m_pWorker = std::make_unique<NdiSenderWorker>(this);
    }
    m_pWorker->setSender(m_pSender->sender);
    if (!m_pWorker->isRunning()) {
        m_pWorker->start(QThread::HighPriority);
    }

    qInfo() << "NdiOutput: publishing NDI® source" << sourceName();
#else
    qInfo() << "NdiOutput: enabled, but build has no NDI support (configure with -DNDI=ON)";
#endif
}

void NdiOutput::stopSender() {
#ifdef __MIXXXXX_NDI__
    if (m_pWorker) {
        m_pWorker->requestStop();
        m_pWorker->wait(3000);
        m_pWorker.reset();
    }

    if (!m_pSender) {
        return;
    }

    if (m_pSender->sender) {
        const NDIlib_v5* ndi = NdiRuntime::instance().api();
        if (ndi != nullptr) {
            ndi->send_destroy(m_pSender->sender);
        }
        m_pSender->sender = nullptr;
        NdiRuntime::instance().release();
    }
    m_frameIndex = 0;
#endif
}

void NdiOutput::slotTick() {
    if (!m_pEnabled || m_pEnabled->get() <= 0.0) {
        return;
    }
    captureAndQueueFrame();
}

void NdiOutput::captureAndQueueFrame() {
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
    if (!m_pSender || !m_pSender->sender || !m_pWorker) {
        return;
    }

    NdiPendingFrame pending;
    pending.width = frame.width();
    pending.height = frame.height();
    pending.strideBytes = frame.bytesPerLine();
    pending.timecode = m_frameIndex++;
    pending.pixels = NdiFrameUtil::bgraBytes(frame);
    if (pending.pixels.isEmpty()) {
        return;
    }

    m_pWorker->enqueueFrame(std::move(pending));
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

} // namespace mixxx
