#pragma once

#include <QImage>
#include <QString>

// Beat-locked video effects driven by [ChannelN],beat_distance and beat grid COs.
// Applied from VideoMixer so FX survive into the master output panel.
class VideoFxChain {
  public:
    // Map CO value (0-5) to beat division 1,2,4,8,16,32.
    static int divisionFromCoValue(double value);

    // Absolute beat index from beat_closest + bpm (seek-stable, no edge accumulation).
    static int beatIndexFromGroup(const QString& group);

    // True when FX should fire on this beat (every N beats).
    static bool beatFxActive(int beatIndex, int division);

    // Strobe intensity 0..1 within the beat window; 0 outside.
    static double strobeStrength(double beatDistance, double bpm);

    // Zoom scale >= 1.0, peaks at beat onset.
    static double zoomScale(double beatDistance, double zoomAmount);

    static QImage applyStrobe(const QImage& src, double strength);
    static QImage applyZoomPump(const QImage& src, double beatDistance, double zoomAmount);

    // Read per-deck COs and apply enabled beat FX to a frame.
    static QImage applyForDeck(int deck, const QImage& src);
};
