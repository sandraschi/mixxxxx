#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>
#include <QTimer>
#include <memory>

#include "track/track.h"
#include "library/coverart.h"

class VideoDecoder;
class ControlPushButton;
class ControlObject;
class ControlPotmeter;

class VideoWidget : public QWidget {
    Q_OBJECT
  public:
    explicit VideoWidget(const QString& group, QWidget* parent = nullptr);
    ~VideoWidget() override;

    void setGroup(const QString& group) { m_group = group; }

  public slots:
    void slotLoadTrack(TrackPointer pTrack);
    void slotVideoEnabled(double v);
    void slotVideoFullscreen(double v);

  signals:
    void fullscreenToggled(bool enabled);

  protected:
    void paintEvent(QPaintEvent* event) override;
    void renderImage(QPainter& p, const QImage& img);

  private slots:
    void slotFrameDecoded(const QImage& frame, double pts);
    void slotPlaybackEnded();
    void slotTick();
    void slotCoverFound(const QObject* requester,
            const CoverInfo& coverInfo,
            const QPixmap& pixmap);

  private:
    void findCompanionVideo(const QString& audioPath);
    int deckIndex() const;
    void stopFallbackVisuals();
    void stopKenBurnsFallback();
    void stopGenerativeFallback();
    void stopPoolLoopFallback();
    bool tryStartFallbackChain();
    bool tryStartPoolLoop();
    bool tryStartGenerativeFallback();
    void tryStartKenBurnsFallback();
    void updateGenerativeFallbackFrame();
    void updateKenBurnsFallbackFrame();
    void ensureDecoder();

    QString m_group;
    TrackPointer m_pTrack;
    VideoDecoder* m_decoder = nullptr;

    std::unique_ptr<ControlPushButton> m_pVideoEnabled;
    std::unique_ptr<ControlPushButton> m_pVideoFullscreen;
    std::unique_ptr<ControlPotmeter> m_pVideoBrightness;
    std::unique_ptr<ControlPotmeter> m_pVideoContrast;
    std::unique_ptr<ControlPotmeter> m_pVideoSaturation;
    std::unique_ptr<ControlPushButton> m_pBeatFxStrobe;
    std::unique_ptr<ControlPushButton> m_pBeatFxZoom;
    std::unique_ptr<ControlPotmeter> m_pBeatFxDivision;
    std::unique_ptr<ControlPotmeter> m_pBeatFxStrobeAmount;
    std::unique_ptr<ControlPotmeter> m_pBeatFxZoomAmount;
    std::unique_ptr<ControlPushButton> m_pVideoFallback;
    std::unique_ptr<ControlPushButton> m_pVideoFallbackGenerative;

    // [Mixer]-scoped, not per-channel - only the "[Master]" VideoWidget
    // instance constructs these (see the constructor), matching how
    // "[Master]" is already the one privileged instance for output-wide
    // controls. Deliberately NOT owned by VideoMixer's singleton: that
    // ownership shape is what caused the previous video_crossfader CO to
    // dangle across per-test ControlObject teardown (see TODO 23 / STATUS.md
    // section 7.1) - VideoWidget has a normal, non-singleton lifecycle, so
    // its unique_ptr-owned COs are destroyed at the same time the CO
    // registry expects them to be.
    std::unique_ptr<ControlPotmeter> m_pVideoCrossfader;
    std::unique_ptr<ControlPushButton> m_pVideoCrossfaderLink;

    QImage m_fallbackCover;
    bool m_usingKenBurnsFallback = false;
    bool m_usingGenerativeFallback = false;
    bool m_usingPoolLoop = false;
    QString m_poolLoopPath;
    double m_poolLoopBpm = 0.0;
    QImage m_currentFrame;
    QMutex m_frameMutex;
    bool m_hasVideo = false;
    QTimer* m_repaintTimer = nullptr;

    QString m_currentVideoPath;
    QWidget* m_fullscreenWindow = nullptr;
};
