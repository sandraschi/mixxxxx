#pragma once

#include <QImage>
#include <QMutex>
#include <QMap>
#include <memory>

class ControlObject;
class VideoDecoder;

// Singleton that composites video frames from all decks
// based on crossfader position and per-deck VFX parameters.
class VideoMixer {
  public:
    static VideoMixer& instance();

    void registerDecoder(int deck, VideoDecoder* decoder);
    void unregisterDecoder(int deck);

    // Push latest frame from a decoder thread
    void pushFrame(int deck, const QImage& frame, double pts);

    // Get blended output frame for the crossfader position
    // deckA/deckB are detected from crossfader position
    QImage blendFrame(double crossfader);

    // VFX helpers — public for per-deck widgets
    static QImage applyBrightnessContrast(const QImage& src, double brightness, double contrast);
    static QImage applySaturation(const QImage& src, double saturation);

  private:
    VideoMixer() = default;
    ~VideoMixer() = default;
    VideoMixer(const VideoMixer&) = delete;

    struct DeckFrame {
        QImage frame;
        double pts = 0.0;
        bool active = false;
    };

    QMutex m_mutex;
    QMap<int, DeckFrame> m_frames;
};

// Free function, not a VideoMixer member, on purpose: blendFrame() above
// stays a pure function with zero ControlObject dependency (the existing
// tests rely on that purity, calling it directly with literal doubles). This
// helper does the [Mixer],video_crossfader / video_crossfader_link lookups
// for the two call sites that need the *effective* crossfader position
// (audio-linked, or the independent one) without making VideoMixer itself
// own or depend on any ControlObject state. See videowidget.h's comment on
// why those COs are owned by the "[Master]" VideoWidget instance rather
// than by VideoMixer's singleton.
double effectiveVideoCrossfaderPosition();
