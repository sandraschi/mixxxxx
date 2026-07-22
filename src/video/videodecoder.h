#pragma once

#include <QImage>
#include <QString>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <memory>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
#include <libavutil/hwcontext.h>
}

class VideoDecoder : public QThread {
    Q_OBJECT
  public:
    explicit VideoDecoder(QObject* parent = nullptr);
    ~VideoDecoder() override;

    void openFile(const QString& filePath);
    void close();
    void pause();
    void resume();
    void seek(double position); // 0.0-1.0

    bool isOpen() const { return m_open; }
    bool isPlaying() const { return m_playing; }
    double durationSeconds() const { return m_duration; }
    int frameWidth() const { return m_width; }
    int frameHeight() const { return m_height; }
    double frameRate() const { return m_frameRate; }
    double currentPosition() const;

    void setSpeed(double speed); // audio-driven speed multiplier
    void setAudioClock(double clockSeconds); // push current audio position

  signals:
    void frameDecoded(const QImage& frame, double pts);
    void playbackEnded();
    void openFailed(const QString& reason);

  protected:
    void run() override;

  private:
    bool initHardwareDecoder();
    bool decodePacket();
    QImage convertFrameToImage(const AVFrame* frame);

    QString m_filePath;

    AVFormatContext* m_formatCtx = nullptr;
    AVCodecContext* m_codecCtx = nullptr;
    const AVCodec* m_codec = nullptr;
    SwsContext* m_swsCtx = nullptr;
    int m_videoStreamIndex = -1;
    AVBufferRef* m_hwDeviceCtx = nullptr;
    AVHWDeviceType m_hwType = AV_HWDEVICE_TYPE_NONE;
    bool m_hwEnabled = false;

    int m_width = 0;
    int m_height = 0;
    double m_duration = 0.0;
    double m_frameRate = 30.0;

    QMutex m_mutex;
    QWaitCondition m_cond;
    bool m_open = false;
    QAtomicInt m_playing{false};
    QAtomicInt m_abort{false};

    double m_speed = 1.0;
    double m_audioClock = 0.0;
    double m_lastPts = 0.0;

    AVPacket* m_packet = nullptr;
    AVFrame* m_frame = nullptr;
};
