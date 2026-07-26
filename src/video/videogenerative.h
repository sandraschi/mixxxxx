#pragma once

#include <QImage>
#include <QSize>

// Beat-reactive procedural fallback (IDEAS.md chain step 3 MVP).
// Full Butter Churn remains in mixx-dj-mcp webapp; this is a native QImage path.
class VideoGenerative {
  public:
    struct Params {
        double beatDistance = 0.0; // 0..1 phase within beat from beat_distance CO
        double bpm = 120.0;
        int beatIndex = 0;
        double energy = 0.5; // 0..1 visual intensity
    };

    static QImage render(const Params& params,
            const QSize& outputSize = QSize(1280, 720));
};
