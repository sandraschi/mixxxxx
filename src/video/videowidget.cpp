#include "video/videowidget.h"
#include "video/videodecoder.h"
#include "control/controlpushbutton.h"
#include "control/controlobject.h"
#include "moc_videowidget.cpp"

#include <QFileInfo>
#include <QDir>
#include <QGuiApplication>
#include <QScreen>
#include <QDebug>

VideoWidget::VideoWidget(const QString& group, QWidget* parent)
    : QOpenGLWidget(parent),
      m_group(group) {
    m_pVideoEnabled = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_enabled"));
    m_pVideoFullscreen = std::make_unique<ControlPushButton>(
        ConfigKey(group, "video_fullscreen"));

    connect(m_pVideoEnabled.get(), &ControlPushButton::valueChanged,
            this, &VideoWidget::slotVideoEnabled);
    connect(m_pVideoFullscreen.get(), &ControlPushButton::valueChanged,
            this, &VideoWidget::slotVideoFullscreen);
}

VideoWidget::~VideoWidget() {
    if (m_decoder) {
        m_decoder->close();
        m_decoder->deleteLater();
        m_decoder = nullptr;
    }
    makeCurrent();
    if (m_textureId) {
        glDeleteTextures(1, &m_textureId);
    }
}

void VideoWidget::slotLoadTrack(const QString& trackPath) {
    findCompanionVideo(trackPath);
}

void VideoWidget::findCompanionVideo(const QString& audioPath) {
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
        m_decoder->openFile(m_currentVideoPath);
        m_hasVideo = true;
        show();
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
    m_textureDirty = true;
    lock.unlock();
    update();
}

void VideoWidget::slotPlaybackEnded() {
    m_hasVideo = false;
    update();
}

void VideoWidget::initializeGL() {
    initializeOpenGLFunctions();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glEnable(GL_TEXTURE_2D);
    glGenTextures(1, &m_textureId);
}

void VideoWidget::resizeGL(int w, int h) {
    glViewport(0, 0, w, h);
}

void VideoWidget::paintGL() {
    glClear(GL_COLOR_BUFFER_BIT);

    if (!m_hasVideo || m_currentFrame.isNull()) {
        return;
    }

    updateTexture();

    glBindTexture(GL_TEXTURE_2D, m_textureId);

    float aspect = static_cast<float>(m_currentFrame.width()) / m_currentFrame.height();
    float winAspect = static_cast<float>(width()) / height();

    float w, h;
    if (aspect > winAspect) {
        w = 1.0f;
        h = winAspect / aspect;
    } else {
        h = 1.0f;
        w = aspect / winAspect;
    }

    glBegin(GL_QUADS);
    glTexCoord2f(0, 0); glVertex2f(-w, h);
    glTexCoord2f(1, 0); glVertex2f(w, h);
    glTexCoord2f(1, 1); glVertex2f(w, -h);
    glTexCoord2f(0, 1); glVertex2f(-w, -h);
    glEnd();
}

void VideoWidget::updateTexture() {
    QMutexLocker lock(&m_frameMutex);
    if (!m_textureDirty) return;

    glBindTexture(GL_TEXTURE_2D, m_textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                 m_currentFrame.width(), m_currentFrame.height(),
                 0, GL_RGBA, GL_UNSIGNED_BYTE,
                 m_currentFrame.bits());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    m_textureDirty = false;
}
