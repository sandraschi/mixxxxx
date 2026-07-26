#include "video/videogenerative.h"

#include <QPainter>
#include <QRadialGradient>
#include <QtMath>

namespace {

QColor hueBackground(int beatIndex, double energy) {
    const double hue = std::fmod(beatIndex * 37.0, 360.0);
    const int saturation = static_cast<int>(120 + energy * 80.0);
    const int value = static_cast<int>(28 + energy * 22.0);
    return QColor::fromHsv(static_cast<int>(hue), qBound(0, saturation, 255),
            qBound(0, value, 255));
}

} // namespace

QImage VideoGenerative::render(const Params& params, const QSize& outputSize) {
    if (outputSize.isEmpty()) {
        return QImage();
    }

    const double energy = qBound(0.0, params.energy, 1.0);
    const double beatDistance = qBound(0.0, params.beatDistance, 1.0);
    const double bpm = params.bpm > 0.0 ? params.bpm : 120.0;

    QImage canvas(outputSize, QImage::Format_RGBA8888);
    canvas.fill(hueBackground(params.beatIndex, energy));

    QPainter painter(&canvas);
    painter.setRenderHint(QPainter::Antialiasing);

    const int w = outputSize.width();
    const int h = outputSize.height();
    const QPointF center(w * 0.5, h * 0.52);

    // Kick pulse: ring expands through the beat (MilkDrop-ish, CPU cheap via QPainter).
    const double pulse = 1.0 - beatDistance;
    const double baseRadius = qMin(w, h) * (0.12 + 0.28 * pulse * energy);
    QRadialGradient glow(center, baseRadius * 1.4);
    const QColor core = QColor::fromHsv(
            static_cast<int>(std::fmod(params.beatIndex * 53.0 + pulse * 40.0, 360.0)),
            200,
            static_cast<int>(160 + 95 * pulse * energy));
    glow.setColorAt(0.0, core);
    glow.setColorAt(0.55, QColor(core.red(), core.green(), core.blue(), 90));
    glow.setColorAt(1.0, Qt::transparent);
    painter.setBrush(glow);
    painter.setPen(Qt::NoPen);
    painter.drawEllipse(center, baseRadius, baseRadius);

    // Beat bars across lower third — phase walks with BPM even when beat_distance stalls.
    const int barCount = 24;
    const double barWidth = static_cast<double>(w) / barCount;
    const double barPhase = std::fmod(
            beatDistance + params.beatIndex * 0.17 + bpm * 0.001, 1.0);
    painter.setBrush(QColor(255, 255, 255, static_cast<int>(40 + 120 * energy)));
    for (int i = 0; i < barCount; ++i) {
        const double t = static_cast<double>(i) / barCount;
        const double wave = 0.35 + 0.65 * qAbs(qSin((t + barPhase) * 2.0 * M_PI * 3.0));
        const double barHeight = h * 0.22 * wave * (0.35 + 0.65 * energy);
        const QRectF bar(i * barWidth + barWidth * 0.15,
                h - barHeight - h * 0.06,
                barWidth * 0.7,
                barHeight);
        painter.drawRoundedRect(bar, 3.0, 3.0);
    }

    // Secondary ring on downbeat (every 4 beats).
    if ((params.beatIndex % 4) == 0 && beatDistance < 0.12) {
        const double flash = 1.0 - (beatDistance / 0.12);
        painter.setPen(QPen(QColor(255, 255, 255, static_cast<int>(180 * flash * energy)), 4));
        painter.setBrush(Qt::NoBrush);
        const double ringR = qMin(w, h) * (0.35 + 0.15 * (1.0 - flash));
        painter.drawEllipse(center, ringR, ringR);
    }

    painter.end();
    return canvas;
}
