#pragma once

#include <QWidget>
#include <QImage>
#include <QMutex>
#include <QTimer>
#include <memory>

#include "track/track.h"

class VideoDecoder;
class ControlPushButton;
class ControlObject;

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

  private slots:
    void slotFrameDecoded(const QImage& frame, double pts);
    void slotPlaybackEnded();
    void slotTick();

  private:
    void findCompanionVideo(const QString& audioPath);

    QString m_group;
    VideoDecoder* m_decoder = nullptr;

    std::unique_ptr<ControlPushButton> m_pVideoEnabled;
    std::unique_ptr<ControlPushButton> m_pVideoFullscreen;

    QImage m_currentFrame;
    QMutex m_frameMutex;
    bool m_hasVideo = false;
    QTimer* m_repaintTimer = nullptr;

    QString m_currentVideoPath;
    QWidget* m_fullscreenWindow = nullptr;
};
