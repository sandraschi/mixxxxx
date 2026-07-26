#include "video/videowidget.h"
#include "video/videodecoder.h"
#include "video/videomixer.h"
#include "video/videofxchain.h"
#include "control/controlpushbutton.h"
#include "control/controlobject.h"
#include "control/controlpotmeter.h"
#include "util/controlcli.h"
#include "moc_videowidget.cpp"

#include <cmath>

#include <QPainter>
#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QPixmap>
#include <QRegularExpression>
#include <QScreen>
#include <QVBoxLayout>
#include "library/coverartcache.h"
#include "library/coverart.h"
#include "track/track.h"
#include "video/videofallback.h"
#include "video/videopool.h"

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

    m_pVideoFallback = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_fallback"), true, 1.0);

    CoverArtCache* pCoverCache = CoverArtCache::instance();
    if (pCoverCache) {
        connect(pCoverCache,
                &CoverArtCache::coverFound,
                this,
                &VideoWidget::slotCoverFound);
    }

    m_repaintTimer = new QTimer(this);
    connect(m_repaintTimer, &QTimer::timeout, this, &VideoWidget::slotTick);
    m_repaintTimer->start(33); // ~30 fps repaint
}

VideoWidget::~VideoWidget() {
    m_repaintTimer->stop();
    stopFallbackVisuals();
    if (m_decoder) {
        m_decoder->close();
        m_decoder->deleteLater();
        m_decoder = nullptr;
    }
}

void VideoWidget::slotLoadTrack(TrackPointer pTrack) {
    m_pTrack = pTrack;
    if (pTrack) {
        stopFallbackVisuals();
        m_fallbackCover = QImage();
        m_poolLoopPath.clear();
        m_poolLoopBpm = 0.0;
        findCompanionVideo(pTrack->getLocation());
        if (m_currentVideoPath.isEmpty()) {
            tryStartFallbackChain();
        }
    } else {
        stopFallbackVisuals();
        m_fallbackCover = QImage();
        m_poolLoopPath.clear();
        m_poolLoopBpm = 0.0;
        m_currentVideoPath.clear();
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
            stopFallbackVisuals();
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
            stopFallbackVisuals();
            if (m_pVideoEnabled->get()) {
                slotVideoEnabled(1.0);
            }
            return;
        }
    }
    m_currentVideoPath.clear();
    stopFallbackVisuals();
    if (m_pVideoEnabled && m_pVideoEnabled->get() > 0.0) {
        tryStartFallbackChain();
    }
}

void VideoWidget::ensureDecoder() {
    if (m_decoder) {
        return;
    }
    m_decoder = new VideoDecoder(this);
    connect(m_decoder, &VideoDecoder::frameDecoded,
            this, &VideoWidget::slotFrameDecoded);
    connect(m_decoder, &VideoDecoder::playbackEnded,
            this, &VideoWidget::slotPlaybackEnded);
}

int VideoWidget::deckIndex() const {
    static const QRegularExpression kDeckGroup(
            QStringLiteral("^(\\[Channel(\\d+)\\])$"));
    const QRegularExpressionMatch match = kDeckGroup.match(m_group);
    return match.hasMatch() ? match.captured(1).toInt() : -1;
}

void VideoWidget::stopFallbackVisuals() {
    stopPoolLoopFallback();
    stopKenBurnsFallback();
}

void VideoWidget::stopKenBurnsFallback() {
    if (!m_usingKenBurnsFallback) {
        return;
    }
    const int deck = deckIndex();
    if (deck > 0 && !m_usingPoolLoop) {
        VideoMixer::instance().unregisterDecoder(deck);
    }
    m_usingKenBurnsFallback = false;
}

void VideoWidget::stopPoolLoopFallback() {
    if (!m_usingPoolLoop) {
        return;
    }
    if (m_decoder) {
        m_decoder->close();
    }
    m_usingPoolLoop = false;
    m_poolLoopPath.clear();
    m_poolLoopBpm = 0.0;
}

bool VideoWidget::tryStartFallbackChain() {
    if (!m_pVideoFallback || m_pVideoFallback->get() <= 0.0) {
        return false;
    }
    if (!m_currentVideoPath.isEmpty() || !m_pTrack) {
        return false;
    }
    if (tryStartPoolLoop()) {
        return true;
    }
    tryStartKenBurnsFallback();
    return m_usingKenBurnsFallback;
}

bool VideoWidget::tryStartPoolLoop() {
    if (!m_pTrack || !m_currentVideoPath.isEmpty()) {
        return false;
    }

    const std::optional<VideoPoolEntry> match = VideoPool::instance().selectBest(
            m_pTrack->getBpm(),
            m_pTrack->getGenre());
    if (!match.has_value()) {
        return false;
    }

    stopKenBurnsFallback();
    ensureDecoder();
    m_decoder->setGroup(m_group);
    m_decoder->setSyncMode(VideoSyncMode::PoolLoop);
    m_decoder->setPoolLoopBpm(match->bpm);
    m_decoder->openFile(match->path);
    if (!m_decoder->isOpen()) {
        m_decoder->setSyncMode(VideoSyncMode::Companion);
        m_decoder->setPoolLoopBpm(0.0);
        return false;
    }

    m_poolLoopPath = match->path;
    m_poolLoopBpm = match->bpm;
    m_usingPoolLoop = true;
    m_hasVideo = true;
    update();
    return true;
}

void VideoWidget::tryStartKenBurnsFallback() {
    if (!m_pVideoFallback || m_pVideoFallback->get() <= 0.0) {
        return;
    }
    if (!m_currentVideoPath.isEmpty() || !m_pTrack || m_usingPoolLoop) {
        return;
    }
    if (m_fallbackCover.isNull()) {
        CoverArtCache::requestUncachedCover(this, m_pTrack, 1024);
        return;
    }
    const int deck = deckIndex();
    if (deck > 0) {
        VideoMixer::instance().registerDecoder(deck, nullptr);
    }
    m_usingKenBurnsFallback = true;
    m_hasVideo = true;
    updateKenBurnsFallbackFrame();
}

void VideoWidget::updateKenBurnsFallbackFrame() {
    if (!m_usingKenBurnsFallback || m_fallbackCover.isNull()) {
        return;
    }
    const double clock = ControlObject::get(ConfigKey(m_group, "video_audio_clock"));
    const double phase = clock > 0.0 ? std::fmod(clock / 30.0, 1.0) : 0.0;
    QImage frame = VideoFallback::renderKenBurns(m_fallbackCover, phase);
    const int deck = deckIndex();
    if (deck > 0) {
        VideoMixer::instance().pushFrame(deck, frame, clock);
    }
    QMutexLocker lock(&m_frameMutex);
    m_currentFrame = frame;
}

void VideoWidget::slotCoverFound(const QObject* requester,
        const CoverInfo& coverInfo,
        const QPixmap& pixmap) {
    Q_UNUSED(coverInfo);
    if (requester != this || pixmap.isNull()) {
        return;
    }
    m_fallbackCover = pixmap.toImage();
    if (m_pVideoEnabled && m_pVideoEnabled->get() > 0.0 && m_currentVideoPath.isEmpty()) {
        tryStartFallbackChain();
    }
}

void VideoWidget::slotVideoEnabled(double v) {
    if (v > 0.0) {
        if (!m_currentVideoPath.isEmpty()) {
            stopFallbackVisuals();
            ensureDecoder();
            m_decoder->setGroup(m_group);
            m_decoder->setSyncMode(VideoSyncMode::Companion);
            m_decoder->setPoolLoopBpm(0.0);
            m_decoder->openFile(m_currentVideoPath);
            m_hasVideo = true;
            update();
            return;
        }
        tryStartFallbackChain();
        update();
    } else {
        stopFallbackVisuals();
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
    if (m_usingKenBurnsFallback) {
        updateKenBurnsFallbackFrame();
    }
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
        } else if (m_pVideoFallback && m_pVideoFallback->get() > 0.0 && m_pTrack &&
                !m_usingPoolLoop) {
            p.setPen(Qt::gray);
            p.drawText(rect(), Qt::AlignCenter, tr("Loading fallback..."));
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

    const int deck = deckIndex();
    if (deck > 0) {
        frame = VideoFxChain::applyForDeck(deck, frame);
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
