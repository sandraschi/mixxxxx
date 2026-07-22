#include "video/videothumbnail.h"
#include "moc_videothumbnail.cpp"

#include <QDir>
#include <QStandardPaths>
#include <QCryptographicHash>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>
}

VideoThumbnail& VideoThumbnail::instance() {
    static VideoThumbnail inst;
    return inst;
}

VideoThumbnail::VideoThumbnail()
    : m_cache(500) { // max 500 entries
}

QImage VideoThumbnail::getThumbnail(const QString& videoPath, int width, int height) {
    QString key = cacheKey(videoPath, width, height);
    if (auto* cached = m_cache.object(key))
        return *cached;

    QImage thumb = extractFrame(videoPath, width, height);
    if (!thumb.isNull())
        m_cache.insert(key, new QImage(thumb));
    return thumb;
}

void VideoThumbnail::clearCache() {
    m_cache.clear();
}

QString VideoThumbnail::cacheKey(const QString& videoPath, int w, int h) const {
    QByteArray data = videoPath.toUtf8() + QByteArray::number(w) + 'x' + QByteArray::number(h);
    return QString::fromUtf8(QCryptographicHash::hash(data, QCryptographicHash::Md5).toHex());
}

QImage VideoThumbnail::extractFrame(const QString& videoPath, int width, int height) {
    AVFormatContext* fmtCtx = nullptr;
    AVCodecContext* codecCtx = nullptr;
    SwsContext* swsCtx = nullptr;
    QImage result;

    if (avformat_open_input(&fmtCtx, videoPath.toUtf8().constData(), nullptr, nullptr) != 0)
        return result;
    if (avformat_find_stream_info(fmtCtx, nullptr) < 0) {
        avformat_close_input(&fmtCtx);
        return result;
    }

    int vidIdx = av_find_best_stream(fmtCtx, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (vidIdx < 0) {
        avformat_close_input(&fmtCtx);
        return result;
    }

    const AVCodec* codec = avcodec_find_decoder(fmtCtx->streams[vidIdx]->codecpar->codec_id);
    if (!codec) {
        avformat_close_input(&fmtCtx);
        return result;
    }

    codecCtx = avcodec_alloc_context3(codec);
    avcodec_parameters_to_context(codecCtx, fmtCtx->streams[vidIdx]->codecpar);
    if (avcodec_open2(codecCtx, codec, nullptr) < 0) {
        avcodec_free_context(&codecCtx);
        avformat_close_input(&fmtCtx);
        return result;
    }

    // Seek to keyframe near 10% into the video (avoids black frames at start)
    int64_t seekTs = fmtCtx->duration / 10;
    av_seek_frame(fmtCtx, vidIdx, seekTs, AVSEEK_FLAG_BACKWARD);

    AVPacket* pkt = av_packet_alloc();
    AVFrame* frame = av_frame_alloc();
    bool gotFrame = false;

    while (av_read_frame(fmtCtx, pkt) == 0) {
        if (pkt->stream_index != vidIdx) {
            av_packet_unref(pkt);
            continue;
        }

        if (avcodec_send_packet(codecCtx, pkt) == 0) {
            if (avcodec_receive_frame(codecCtx, frame) == 0) {
                swsCtx = sws_getContext(codecCtx->width, codecCtx->height,
                    AV_PIX_FMT_YUV420P, width, height, AV_PIX_FMT_RGBA,
                    SWS_BILINEAR, nullptr, nullptr, nullptr);
                if (swsCtx) {
                    result = QImage(width, height, QImage::Format_RGBA8888);
                    uint8_t* dst[] = { result.bits() };
                    int stride[] = { static_cast<int>(result.bytesPerLine()) };
                    sws_scale(swsCtx, frame->data, frame->linesize, 0,
                              codecCtx->height, dst, stride);
                    sws_freeContext(swsCtx);
                }
                gotFrame = true;
                break;
            }
        }
        av_packet_unref(pkt);
    }

    av_packet_free(&pkt);
    av_frame_free(&frame);
    avcodec_free_context(&codecCtx);
    avformat_close_input(&fmtCtx);

    return result;
}
