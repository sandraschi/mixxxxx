#pragma once

#include <QImage>
#include <QString>
#include <QCache>

class VideoThumbnail {
  public:
    static VideoThumbnail& instance();

    QImage getThumbnail(const QString& videoPath, int width = 320, int height = 180);
    void clearCache();

  private:
    VideoThumbnail();
    QImage extractFrame(const QString& videoPath, int width, int height);
    QString cacheKey(const QString& videoPath, int w, int h) const;

    QCache<QString, QImage> m_cache;
};
