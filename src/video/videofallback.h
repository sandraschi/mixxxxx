#pragma once

#include <QImage>
#include <QSize>

// Fallback visuals when no companion video file exists (IDEAS.md chain step 4).
class VideoFallback {
  public:
    // phase01 wraps 0..1 over a ~30s Ken Burns cycle.
    static QImage renderKenBurns(const QImage& cover,
            double phase01,
            const QSize& outputSize = QSize(1280, 720));
};
