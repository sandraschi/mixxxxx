#include "video/videofxchain.h"

#include "control/controlobject.h"

#include <QPainter>
#include <QtMath>

namespace {

constexpr int kDivisions[] = {1, 2, 4, 8, 16, 32};
constexpr int kDivisionCount = sizeof(kDivisions) / sizeof(kDivisions[0]);
constexpr double kMinStrobeWindowMs = 66.0; // ~2 frames at 30fps

double controlOrZero(const ConfigKey& key) {
    const double value = ControlObject::get(key);
    return std::isfinite(value) ? value : 0.0;
}

} // namespace

int VideoFxChain::divisionFromCoValue(double value) {
    const int index = qBound(0, static_cast<int>(qRound(value)), kDivisionCount - 1);
    return kDivisions[index];
}

int VideoFxChain::beatIndexFromGroup(const QString& group) {
    const double beatClosest = controlOrZero(ConfigKey(group, "beat_closest"));
    const double bpm = controlOrZero(ConfigKey(group, "bpm"));
    const double sampleRate = controlOrZero(ConfigKey("[App]", "samplerate"));
    if (beatClosest < 0.0 || bpm <= 0.0 || sampleRate <= 0.0) {
        return 0;
    }
    const double samplesPerBeat = sampleRate * 60.0 / bpm;
    if (samplesPerBeat <= 0.0) {
        return 0;
    }
    return static_cast<int>(qFloor(beatClosest / samplesPerBeat));
}

bool VideoFxChain::beatFxActive(int beatIndex, int division) {
    if (division <= 1) {
        return true;
    }
    return (beatIndex % division) == 0;
}

double VideoFxChain::strobeStrength(double beatDistance, double bpm) {
    if (beatDistance < 0.0) {
        return 0.0;
    }
    if (bpm <= 0.0) {
        bpm = 120.0;
    }
    const double msPerBeat = 60000.0 / bpm;
    const double windowBeats = qMax(0.15, kMinStrobeWindowMs / msPerBeat);
    if (beatDistance > windowBeats) {
        return 0.0;
    }
    return 1.0 - (beatDistance / windowBeats);
}

double VideoFxChain::zoomScale(double beatDistance, double zoomAmount) {
    const double amount = qBound(0.0, zoomAmount, 1.0);
    if (amount <= 0.0) {
        return 1.0;
    }
    const double t = qBound(0.0, beatDistance / 0.5, 1.0);
    return 1.0 + (0.12 * amount) * (1.0 - t);
}

QImage VideoFxChain::applyStrobe(const QImage& src, double strength) {
    if (src.isNull() || strength <= 0.0) {
        return src;
    }

    QImage result = src.format() == QImage::Format_RGBA8888
            ? src.copy()
            : src.convertToFormat(QImage::Format_RGBA8888);
    const int boost = static_cast<int>(strength * 200.0);

    for (int y = 0; y < result.height(); ++y) {
        auto* line = reinterpret_cast<QRgb*>(result.scanLine(y));
        for (int x = 0; x < result.width(); ++x) {
            const int r = qBound(0, qRed(line[x]) + boost, 255);
            const int g = qBound(0, qGreen(line[x]) + boost, 255);
            const int b = qBound(0, qBlue(line[x]) + boost, 255);
            line[x] = qRgba(r, g, b, qAlpha(line[x]));
        }
    }
    return result;
}

QImage VideoFxChain::applyZoomPump(const QImage& src,
        double beatDistance,
        double zoomAmount) {
    if (src.isNull()) {
        return src;
    }

    const double scale = zoomScale(beatDistance, zoomAmount);
    if (qFuzzyCompare(scale, 1.0)) {
        return src;
    }

    QImage result(src.size(), QImage::Format_RGBA8888);
    result.fill(Qt::black);

    QPainter painter(&result);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.translate(result.width() / 2.0, result.height() / 2.0);
    painter.scale(scale, scale);
    painter.drawImage(-src.width() / 2.0, -src.height() / 2.0, src);
    painter.end();

    return result;
}

QImage VideoFxChain::applyForDeck(int deck, const QImage& src) {
    if (src.isNull() || deck <= 0) {
        return src;
    }

    const QString group = QStringLiteral("[Channel%1]").arg(deck);
    const double strobeOn = controlOrZero(ConfigKey(group, "video_beat_fx_strobe"));
    const double zoomOn = controlOrZero(ConfigKey(group, "video_beat_fx_zoom"));
    if (strobeOn <= 0.0 && zoomOn <= 0.0) {
        return src;
    }

    const double beatDistance = controlOrZero(ConfigKey(group, "beat_distance"));
    const double bpm = controlOrZero(ConfigKey(group, "bpm"));
    const int division = divisionFromCoValue(
            controlOrZero(ConfigKey(group, "video_beat_fx_division")));
    const int beatIndex = beatIndexFromGroup(group);
    if (!beatFxActive(beatIndex, division)) {
        return src;
    }

    const double strobeAmount = qBound(0.0,
            controlOrZero(ConfigKey(group, "video_beat_fx_strobe_amount")),
            1.0);
    const double zoomAmount = qBound(0.0,
            controlOrZero(ConfigKey(group, "video_beat_fx_zoom_amount")),
            1.0);

    QImage result = src;
    if (strobeOn > 0.0) {
        const double strength = strobeStrength(beatDistance, bpm) * strobeAmount;
        if (strength > 0.0) {
            result = applyStrobe(result, strength);
        }
    }
    if (zoomOn > 0.0 && zoomAmount > 0.0) {
        result = applyZoomPump(result, beatDistance, zoomAmount);
    }
    return result;
}
