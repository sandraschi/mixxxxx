#include "video/videowidget.h"
#include "video/videodecoder.h"
#include "video/videomixer.h"
#include "video/videofxchain.h"
#include "control/controlpushbutton.h"
#include "control/controlobject.h"
#include "control/controlpotmeter.h"
#include "util/controlcli.h"
#include "moc_videowidget.cpp"

#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QRegularExpression>
#include <QScreen>
#include <QVBoxLayout>
#include "track/track.h"

VideoWidget::VideoWidget(const QString& group, QWidget* parent)
    : QWidget(parent),
      m_group(group) {
    setAttribute(Qt::WA_OpaquePaintEvent);
    setMinimumSize(160, 120);

    m_pVideoEnabled = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_enabled"));
    m_pVideoFullscreen = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_fullscreen"));

    connect(m_pVideoEnabled.get(), &ControlPushButton::valueChanged,
            this, &VideoWidget::slotVideoEnabled);
    connect(m_pVideoFullscreen.get(), &ControlPushButton::valueChanged,
            this, &VideoWidget::slotVideoFullscreen);

    m_pVideoBrightness = std::make_unique<ControlPotmeter>(
        ConfigKey(group, "video_brightness"), -1.0, 1.0, true);
    m_pVideoContrast = std::make_unique<ControlPotmeter>(
        ConfigKey(group, "video_contrast"), 0.0, 3.0, true);
    m_pVideoSaturation = std::make_unique<ControlPotmeter>(
        ConfigKey(group, "video_saturation"), 0.0, 3.0, true);

    m_pBeatFxStrobe = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_beat_fx_strobe"), true, 0.0);
    m_pBeatFxZoom = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_beat_fx_zoom"), true, 0.0);
    m_pBeatFxDivision = std::make_unique<ControlPotmeter>(
        ConfigKey(group, "video_beat_fx_division"), 0.0, 5.0, true);
    m_pBeatFxStrobeAmount = std::make_unique<ControlPotmeter>(
        ConfigKey(group, "video_beat_fx_strobe_amount"), 0.0, 1.0, true);
    m_pBeatFxZoomAmount = std::make_unique<ControlPotmeter>(
        ConfigKey(group, "video_beat_fx_zoom_amount"), 0.0, 1.0, true);
    m_pBeatFxStrobeAmount->set(1.0);
    m_pBeatFxZoomAmount->set(1.0);

    m_repaintTimer = new QTimer(this);
    connect(m_repaintTimer, &QTimer::timeout, this, &VideoWidget::slotTick);
    m_repaintTimer->start(33); // ~30 fps repaint
}

VideoWidget::~VideoWidget() {
    m_repaintTimer->stop();
    if (m_decoder) {
        m_decoder->close();
        m_decoder->deleteLater();
        m_decoder = nullptr;
    }
}

void VideoWidget::slotLoadTrack(TrackPointer pTrack) {
    if (pTrack) {
        findCompanionVideo(pTrack->getLocation());
    }
}

void VideoWidget::findCompanionVideo(const QString& audioPath) {
    const QRegularExpression channelPattern(QStringLiteral(R"(\[Channel(\d+)\])"));
    const QRegularExpressionMatch match = channelPattern.match(m_group);
    if (match.hasMatch()) {
        const int deck = match.captured(1).toInt();
        const QString overridePath = mixxx::ControlCli::videoOverrideForDeck(deck);
        if (!overridePath.isEmpty() && QFileInfo::exists(overridePath)) {
            m_currentVideoPath = overridePath;
            if (m_pVideoEnabled->get()) {
                slotVideoEnabled(1.0);
            }
            return;
        }
    }

    QFileInfo fi(audioPath);
    QString basePath = fi.absolutePath() + "/" + fi.completeBaseName();

    QStringList candidates = {".mp4", ".mkv", ".mov", ".webm"};
    for (const auto& ext : candidates) {
        QString videoPath = basePath + ext;
        if (QFileInfo::exists(videoPath)) {
            m_currentVideoPath = videoPath;
            if (m_pVideoEnabled->get()) {
                slotVideoEnabled(1.0);
            }
            return;
        }
    }
    m_currentVideoPath.clear();
}

void VideoWidget::slotVideoEnabled(double v) {
    if (v > 0.0) {
        if (m_currentVideoPath.isEmpty()) return;
        if (!m_decoder) {
            m_decoder = new VideoDecoder(this);
            connect(m_decoder, &VideoDecoder::frameDecoded,
                    this, &VideoWidget::slotFrameDecoded);
            connect(m_decoder, &VideoDecoder::playbackEnded,
                    this, &VideoWidget::slotPlaybackEnded);
        }
        m_decoder->setGroup(m_group);
        m_decoder->openFile(m_currentVideoPath);
        m_hasVideo = true;
        update();
    } else {
        if (m_decoder) {
            m_decoder->pause();
        }
        m_hasVideo = false;
    }
}

void VideoWidget::slotVideoFullscreen(double v) {
    if (v > 0.0) {
        if (!m_fullscreenWindow) {
            m_fullscreenWindow = new QWidget(nullptr, Qt::Window);
            auto* layout = new QVBoxLayout(m_fullscreenWindow);
            auto* fsWidget = new VideoWidget(m_group, m_fullscreenWindow);
            fsWidget->m_currentVideoPath = m_currentVideoPath;
            fsWidget->m_hasVideo = m_hasVideo;
            if (m_decoder) {
                connect(m_decoder, &VideoDecoder::frameDecoded,
                        fsWidget, &VideoWidget::slotFrameDecoded);
            }
            layout->addWidget(fsWidget);
            m_fullscreenWindow->showFullScreen();
        }
    } else {
        if (m_fullscreenWindow) {
            m_fullscreenWindow->close();
            m_fullscreenWindow->deleteLater();
            m_fullscreenWindow = nullptr;
        }
    }
}

void VideoWidget::slotFrameDecoded(const QImage& frame, double pts) {
    Q_UNUSED(pts);
    QMutexLocker lock(&m_frameMutex);
    m_currentFrame = frame;
}

void VideoWidget::slotTick() {
    if (m_hasVideo && !m_currentFrame.isNull()) {
        update();
    }
}

void VideoWidget::slotPlaybackEnded() {
    m_hasVideo = false;
    update();
}

void VideoWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    if (!m_hasVideo || m_currentFrame.isNull()) {
        // Try to show the blended output from the video mixer
        ControlObject* co = ControlObject::getControl(ConfigKey("[Mixer]", "crossfader"));
        double xfader = co ? co->get() : 0.0;
        QImage mixed = VideoMixer::instance().blendFrame(xfader);
        if (!mixed.isNull()) {
            renderImage(p, mixed);
            return;
        }

        p.fillRect(rect(), Qt::black);
        if (!m_currentVideoPath.isEmpty()) {
            p.setPen(Qt::white);
            p.drawText(rect(), Qt::AlignCenter, tr("Video Paused"));
        } else if (!m_group.isEmpty()) {
            p.setPen(Qt::gray);
            p.drawText(rect(), Qt::AlignCenter, tr("No Video"));
        }
        return;
    }

    QMutexLocker lock(&m_frameMutex);
    QImage frame = m_currentFrame;
    lock.unlock();

    // Apply per-deck VFX
    double bright = m_pVideoBrightness ? m_pVideoBrightness->get() : 0.0;
    double contrast = m_pVideoContrast ? m_pVideoContrast->get() : 1.0;
    double saturation = m_pVideoSaturation ? m_pVideoSaturation->get() : 1.0;
    if (!qFuzzyCompare(bright, 0.0) || !qFuzzyCompare(contrast, 1.0)) {
        frame = VideoMixer::instance().applyBrightnessContrast(frame, bright, contrast);
    }
    if (!qFuzzyCompare(saturation, 1.0)) {
        frame = VideoMixer::instance().applySaturation(frame, saturation);
    }

    const QRegularExpressionMatch deckMatch =
            QRegularExpression(QStringLiteral(R"(\[Channel(\d+)\])")).match(m_group);
    if (deckMatch.hasMatch()) {
        frame = VideoFxChain::applyForDeck(deckMatch.captured(1).toInt(), frame);
    }

    renderImage(p, frame);
}

void VideoWidget::renderImage(QPainter& p, const QImage& img) {
    QRect target;
    double frameAspect = (double)img.width() / img.height();
    double widgetAspect = (double)width() / height();

    if (frameAspect > widgetAspect) {
        int h = (int)(width() / frameAspect);
        target = QRect(0, (height() - h) / 2, width(), h);
    } else {
        int w = (int)(height() * frameAspect);
        target = QRect((width() - w) / 2, 0, w, height());
    }

    p.drawImage(target, img);
}
