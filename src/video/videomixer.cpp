#include "video/videomixer.h"
#include "video/videofxchain.h"
#include "moc_videomixer.cpp"

#include <QPainter>
#include <QtMath>
#include <algorithm>

VideoMixer& VideoMixer::instance() {
    static VideoMixer inst;
    return inst;
}

void VideoMixer::registerDecoder(int deck, VideoDecoder* decoder) {
    Q_UNUSED(decoder);
    QMutexLocker lock(&m_mutex);
    m_frames[deck].active = true;
}

void VideoMixer::unregisterDecoder(int deck) {
    QMutexLocker lock(&m_mutex);
    m_frames.remove(deck);
}

void VideoMixer::pushFrame(int deck, const QImage& frame, double pts) {
    QMutexLocker lock(&m_mutex);
    if (!m_frames.contains(deck) || !m_frames[deck].active)
        return;
    m_frames[deck].frame = frame;
    m_frames[deck].pts = pts;
}

QImage VideoMixer::blendFrame(double crossfader) {
    QList<int> activeDecks;
    QImage frameA;
    QImage frameB;
    int deckA = -1;
    int deckB = -1;

    {
        QMutexLocker lock(&m_mutex);

        for (auto it = m_frames.constBegin(); it != m_frames.constEnd(); ++it) {
            if (it.value().active && !it.value().frame.isNull()) {
                activeDecks.append(it.key());
            }
        }
        std::sort(activeDecks.begin(), activeDecks.end());

        if (activeDecks.size() == 1) {
            deckA = activeDecks.first();
            frameA = m_frames[deckA].frame.copy();
        } else if (activeDecks.size() >= 2) {
            deckA = activeDecks.at(0);
            deckB = activeDecks.at(1);
            frameA = m_frames[deckA].frame.copy();
            frameB = m_frames[deckB].frame.copy();
        }
    }

    if (activeDecks.isEmpty()) {
        return QImage();
    }

    if (activeDecks.size() == 1) {
        return VideoFxChain::applyForDeck(deckA, frameA);
    }

    const double opacityB = qBound(0.0, (crossfader + 1.0) / 2.0, 1.0);

    frameA = VideoFxChain::applyForDeck(deckA, frameA);
    frameB = VideoFxChain::applyForDeck(deckB, frameB);
    QImage result = frameA.copy();

    QPainter p(&result);
    p.setOpacity(opacityB);
    p.drawImage(0, 0, frameB);
    p.end();

    return result;
}

QImage VideoMixer::applyBrightnessContrast(const QImage& src, double brightness, double contrast) {
    if (qFuzzyCompare(brightness, 0.0) && qFuzzyCompare(contrast, 1.0))
        return src;

    QImage result = src.convertToFormat(QImage::Format_RGBA8888);
    int b = qBound(-255, (int)(brightness * 255), 255);
    double c = qMax(0.0, contrast);
    double cf = (259.0 * (c * 128.0 + 128.0)) / (255.0 * (259.0 - c * 128.0 - 128.0));

    for (int y = 0; y < result.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            int r = qBound(0, (int)(cf * (qRed(line[x]) - 128) + 128 + b), 255);
            int g = qBound(0, (int)(cf * (qGreen(line[x]) - 128) + 128 + b), 255);
            int bl = qBound(0, (int)(cf * (qBlue(line[x]) - 128) + 128 + b), 255);
            line[x] = qRgba(r, g, bl, qAlpha(line[x]));
        }
    }
    return result;
}

QImage VideoMixer::applySaturation(const QImage& src, double saturation) {
    if (qFuzzyCompare(saturation, 1.0))
        return src;

    QImage result = src.convertToFormat(QImage::Format_RGBA8888);
    for (int y = 0; y < result.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            int r = qRed(line[x]), g = qGreen(line[x]), b = qBlue(line[x]);
            int gray = (r + g + b) / 3;
            r = qBound(0, (int)(gray + saturation * (r - gray)), 255);
            g = qBound(0, (int)(gray + saturation * (g - gray)), 255);
            b = qBound(0, (int)(gray + saturation * (b - gray)), 255);
            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }
    }
    return result;
}
