#include "video/videomixer.h"
#include "control/controlobject.h"
#include "moc_videomixer.cpp"

#include <QPainter>
#include <QtMath>

VideoMixer& VideoMixer::instance() {
    static VideoMixer inst;
    return inst;
}

VideoMixer::VideoMixer()
    : m_pVideoCrossfader(std::make_unique<ControlObject>(
          ConfigKey("[Mixer]", "video_crossfader"), false, false, false, 0.0)) {
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
    QMutexLocker lock(&m_mutex);

    // Find active decks on each side of the crossfader
    int deckA = -1, deckB = -1;
    QList<int> activeDecks;
    for (auto it = m_frames.constBegin(); it != m_frames.constEnd(); ++it) {
        if (it.value().active && !it.value().frame.isNull())
            activeDecks.append(it.key());
    }

    if (activeDecks.isEmpty())
        return QImage();

    if (activeDecks.size() == 1) {
        deckA = activeDecks[0];
    } else {
        // Crossfader maps to [-1, 1]: -1 = deck 1, 1 = deck 4
        // Map crossfader position to deck pair
        double pos = (crossfader + 1.0) / 2.0; // normalize to 0..1
        int total = activeDecks.size();
        int idx = qBound(0, (int)(pos * (total - 1)), total - 2);
        deckA = activeDecks[idx];
        deckB = activeDecks[qMin(idx + 1, total - 1)];
    }

    DeckFrame& fA = m_frames[deckA];
    QImage result = fA.frame;

    if (deckB >= 0 && m_frames.contains(deckB)) {
        DeckFrame& fB = m_frames[deckB];
        if (!fB.frame.isNull() && fB.active) {
            double pos = (crossfader + 1.0) / 2.0;
            double blend = fabs(crossfader); // 0 at center, 1 at edges

            QPainter p(&result);
            p.setOpacity(blend);
            p.drawImage(0, 0, fB.frame);
            p.end();
        }
    }

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
