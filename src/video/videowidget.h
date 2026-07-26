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
    void stopFallback();
    void tryStartFallback();
    void updateFallbackFrame();

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

    QImage m_fallbackCover;
    bool m_usingFallback = false;
    QImage m_currentFrame;
    QMutex m_frameMutex;
    bool m_hasVideo = false;
    QTimer* m_repaintTimer = nullptr;

    QString m_currentVideoPath;
    QWidget* m_fullscreenWindow = nullptr;
};
