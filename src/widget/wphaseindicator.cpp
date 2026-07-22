#include "widget/wphaseindicator.h"

#include <QPainter>

#include "control/controlproxy.h"
#include "moc_wphaseindicator.cpp"

namespace {
constexpr int kRingWidth = 4;
constexpr int kDefaultSize = 40;
constexpr int kUpdateIntervalMs = 33; // ~30 fps

QColor colorForPhase(float phaseDeg) {
    float t = qMin(1.0f, phaseDeg / 180.0f);
    int r, g, b;
    if (t < 0.5f) {
        float s = t / 0.5f;
        r = static_cast<int>(s * 255.0f);
        g = static_cast<int>((1.0f - s) * 255.0f);
        b = 0;
    } else {
        float s = (t - 0.5f) / 0.5f;
        r = 255;
        g = static_cast<int>((1.0f - s) * 128.0f);
        b = 0;
    }
    return QColor(r, g, b);
}
} // namespace

WPhaseIndicator::WPhaseIndicator(const QString& group, QWidget* parent)
        : QWidget(parent),
          WBaseWidget(this),
          m_group(group),
          m_pPhase(std::make_unique<ControlProxy>(
                  ConfigKey(group, "phase"), this)),
          m_pTimer(new QTimer(this)) {
    setMinimumSize(kDefaultSize, kDefaultSize);
    setSizePolicy(QSizePolicy::MinimumExpanding,
            QSizePolicy::MinimumExpanding);
    connect(m_pTimer, &QTimer::timeout, this, &WPhaseIndicator::updatePhase);
    m_pTimer->start(kUpdateIntervalMs);
    m_elapsed.start();
}

WPhaseIndicator::~WPhaseIndicator() = default;

void WPhaseIndicator::Init() {
}

void WPhaseIndicator::updatePhase() {
    m_phase = static_cast<float>(m_pPhase->get());
    update();
}

void WPhaseIndicator::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    int side = qMin(width(), height());
    int half = side / 2;
    int outer = half - 2;
    int inner = outer - kRingWidth;

    // Dark background circle
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(QColor(30, 30, 40)));
    p.drawEllipse(QPointF(half, half), outer + 1, outer + 1);

    // Arc gap opens at top (12 o'clock)
    float gapDeg = qMax(0.0f, qMin(300.0f, m_phase));
    float spanDeg = 360.0f - gapDeg;

    // Pulsing alpha for badly-out-of-phase (> 180 deg)
    int alpha = 255;
    if (m_phase > 180.0f) {
        double pulse = 0.5 + 0.5 * qSin(m_elapsed.elapsed() / 400.0 * 2.0 * M_PI);
        alpha = static_cast<int>(pulse * 255.0);
    }

    QColor color = colorForPhase(m_phase);
    color.setAlpha(alpha);

    // Draw the ring arc
    QPen pen(QBrush(color), kRingWidth, Qt::SolidLine, Qt::FlatCap);
    p.setPen(pen);
    p.setBrush(Qt::NoBrush);

    // 0 degrees = 3 o'clock in Qt. Arc starts at gap center (12 o'clock = 90 deg).
    int startAngle = static_cast<int>((90.0f + gapDeg / 2.0f) * 16.0f);
    int spanAngle = static_cast<int>(spanDeg * 16.0f);
    if (spanAngle > 0) {
        p.drawArc(QRectF(half - outer, half - outer,
                           outer * 2, outer * 2),
                   startAngle, spanAngle);
    }

    // Center dot
    p.setPen(Qt::NoPen);
    p.setBrush(QBrush(color.darker(150)));
    p.drawEllipse(QPointF(half, half), 2, 2);
}
