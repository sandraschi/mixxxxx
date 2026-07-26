# Mixxxxx — Video-Enabled Mixxx Fork

**Status**: Active development (v0.4 — exporters wired, setAudioClock cleanup)
**Base**: Mixxx 2.5.6
**Version**: v0.4

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
| `phase` | `[Channel{N}]` | 0-360 | Sprint 1 |
| `rekordbox_usb_path` | `[Export]` | string | Sprint 1 |
| `export_crate` | `[Export]` | push button | Sprint 1 |
| `export_rekordbox` | `[Channel{N}]` | push button | Sprint 1 |

### Known Gaps

| Gap | Impact | Effort to Fix |
|-----|--------|---------------|
| ~~**A/V sync unwired** — `setAudioClock()` never called~~ | ~~Scratch/loop/pitch-bend breaks video sync~~ | ✅ **Wired Sprint 3** — VideoSyncControl + 3-tier drift management |
| ~~**Exporters unwired** — trigger COs had no effect~~ | ~~Pressing export did nothing~~ | ✅ **Fixed v0.4** — ExportController connects all 7 COs to exporters |
| **No stem separation** | Can't isolate vocals/drums/bass from video tracks | Medium — ONNX Runtime integration, ~400 lines C++ |
| **No clip extraction** | Can't extract segments from longer videos | Easy — shell out to FFmpeg |
| **No library thumbnails** | Video files show blank cover art in Mixxx library | Easy — wire VideoThumbnail into existing CoverArt DAO |

## Sprint 1 (Shipped)

### Phase Indicator

Reads `[Channel{N}],beat_distance` (0-1) via `PhaseControl::process()` on each engine callback,
converts to degrees, writes `[Channel{N}],phase` (0-360). `WPhaseIndicator` widget reads the CO
at ~30fps via QTimer, renders a QPainter arc ring with color gradient:

- **0°** (aligned): green arc closed, full circle solid green
- **<90°**: green→yellow arc
- **90-180°**: orange arc, gap opening at 12 o'clock
- **>180°**: red arc, pulsing alpha, gap > 50% of circle

Skin element: `<PhaseIndicator>` added to LateNight. Replaces the numeric beat-distance display
with a visual ring.

**Runtime cost**: single `beat_distance` read + multiply per engine callback; paint every 33ms
(when visible). Negligible CPU.

### Rekordbox Export

`RekordboxExporter` class reads Mixxx SQLite library database, writes Pioneer .pdb files via
libdjinterop for USB export. Supports two operations:

- `exportTrack(trackPath, usbPath)` — single track to Pioneer format
- `exportCrate(crateName, usbPath)` — entire crate (all tracks in a Mixxx crate)

ControlObjects:

| CO | Group | Range | Purpose |
|----|-------|-------|---------|
| `rekordbox_usb_path` | `[Export]` | string | Target USB path for Pioneer export |
| `export_crate` | `[Export]` | push | Trigger export of the configured crate |
| `export_rekordbox` | `[Channel{N}]` | push button (per-deck) | Trigger export of current deck's track |

**Dependency**: libdjinterop must be linked at build time. Without it, export calls are
no-ops logged at warning level.

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
| phase | `/deck/{N}/phase` | `mixx_deck(operation="get_phase")` |
| export_rekordbox | `/deck/{N}/export_rekordbox` | `mixx_deck(operation="export_rekordbox")` |

## Non-Goals

- Real-time AI stem separation during playback (pre-process before gig)
- Video transitions/effects beyond crossfader blend
- Video recording or streaming output
- DRM'd video content
- Replacing a dedicated video mixer hardware
