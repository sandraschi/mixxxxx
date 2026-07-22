#pragma once

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QImage>
#include <QMutex>
#include <memory>

class VideoDecoder;
class ControlPushButton;
class ControlObject;

class VideoWidget : public QOpenGLWidget, protected QOpenGLFunctions {
    Q_OBJECT
  public:
    explicit VideoWidget(const QString& group, QWidget* parent = nullptr);
    ~VideoWidget() override;

    void setGroup(const QString& group) { m_group = group; }

  public slots:
    void slotLoadTrack(const QString& trackPath);
    void slotVideoEnabled(double v);
    void slotVideoFullscreen(double v);

  signals:
    void fullscreenToggled(bool enabled);

  protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

  private slots:
    void slotFrameDecoded(const QImage& frame, double pts);
    void slotPlaybackEnded();

  private:
    void findCompanionVideo(const QString& audioPath);
    void updateTexture();

    QString m_group;
    VideoDecoder* m_decoder = nullptr;

    std::unique_ptr<ControlPushButton> m_pVideoEnabled;
    std::unique_ptr<ControlPushButton> m_pVideoFullscreen;

    QImage m_currentFrame;
    QMutex m_frameMutex;
    GLuint m_textureId = 0;
    bool m_textureDirty = false;
    bool m_hasVideo = false;

    QString m_currentVideoPath;
    QWidget* m_fullscreenWindow = nullptr;
};
