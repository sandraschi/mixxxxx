# Mixxxxx — Video-Enabled Mixxx Fork

**Status**: Active development (v0.1 scaffolded)
**Base**: Mixxx 2.5.6
**Version**: v0.1

## What It Is

Mixxxxx is a fork of Mixxx 2.5.6 that adds video playback alongside audio decks. Companion video files (same basename, .mp4/.mkv/.mov/.webm) are autoloaded when a track loads, decoded via bundled FFmpeg.

**Honest status**: The decode, render, compositing, and hardware decode pieces compile and run. What's NOT wired yet: A/V sync (video free-runs on its own timer, doesn't follow deck transport). Suitable for projection/video backdrop. Not suitable for scratch/loop/pitch-locked video.

## What Actually Exists

### Decode (src/video/videodecoder.h/.cpp)

- FFmpeg decode thread per deck via avcodec_send_packet / avcodec_receive_frame
- PTS tracking in `m_lastPts`
- `setAudioClock()` method exists but is NEVER CALLED by the engine (the hard 20% is unwired)
- Video runs on its own msleep timer paced to the file's frame rate
- Loops back to position 0 on EOF — no awareness of deck transport
- Hardware decode via D3D11VA or CUDA fallback in `initHardwareDecoder()` — inline, not a separate module

### Rendering (src/video/videowidget.h/.cpp)

- QPainter-based rendering with aspect-ratio letterbox
- Integrates as `<VideoWidget>` skin element in LateNight
- Fullscreen window support for secondary monitor output
- Per-deck brightness/contrast/saturation via ControlPotmeter COs

### Mixer (src/video/videomixer.h/.cpp)

- Singleton composites frames from all active decks
- Crossfader blending: blends Deck A→Deck B opacity following [Mixer],crossfader
- VFX helpers static — callable from per-deck widgets

### Thumbnails (src/video/videothumbnail.h/.cpp)

- FFmpeg keyframe extraction at 10% seek position
- 500-entry LRU cache
- `getThumbnail(path, w, h)` API

### ControlObjects

| CO | Group | Range | Status |
|----|-------|-------|--------|
| `video_enabled` | `[Channel{N}]` | 0/1 | Working |
| `video_fullscreen` | `[Channel{N}]` | 0/1 | Working |
| `video_brightness` | `[Channel{N}]` | -1 to 1 | Working |
| `video_contrast` | `[Channel{N}]` | 0 to 3 | Working |
| `video_saturation` | `[Channel{N}]` | 0 to 3 | Working |
| `video_crossfader` | `[Mixer]` | -1 to 1 | Working |

### Known Gaps

| Gap | Impact | Effort to Fix |
|-----|--------|---------------|
| **A/V sync unwired** — `setAudioClock()` never called | Scratch/loop/pitch-bend breaks video sync | Hard — needs EngineBuffer hook, backward seek handling, loop stutter avoidance |
| **No stem separation** | Can't isolate vocals/drums/bass from video tracks | Medium — ONNX Runtime integration, ~400 lines C++ |
| **No clip extraction** | Can't extract segments from longer videos | Easy — shell out to FFmpeg |
| **No library thumbnails** | Video files show blank cover art in Mixxx library | Easy — wire VideoThumbnail into existing CoverArt DAO |

## Quick Build

```powershell
tools\windows_release_buildenv.bat
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="..\buildenv\mixxx-deps-2.5-x64-windows-release-40c29ff\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-release -G Ninja ..
ninja
```

## OSC Control (via mixx-dj-mcp)

| CO | OSC Address | mixx-dj-mcp tool |
|----|-------------|------------------|
| video_enabled | `/deck/{N}/video_enabled` | `mixx_deck(operation="video_enable")` |
| video_fullscreen | `/deck/{N}/video_fullscreen` | `mixx_deck(operation="video_fullscreen")` |

## Non-Goals

- Real-time AI stem separation during playback (pre-process before gig)
- Video transitions/effects beyond crossfader blend
- Video recording or streaming output
- DRM'd video content
- Replacing a dedicated video mixer hardware
