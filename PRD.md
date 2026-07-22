# Mixxxxx — Video-Enabled Mixxx Fork

**Status**: Active development
**Base**: Mixxx 2.5.6
**Version**: v2

## What It Is

Mixxxxx is a fork of Mixxx 2.5.6 that adds real-time video playback to the DJ mixer.
Companion video files (same basename, .mp4/.mkv/.mov/.webm) are autoloaded when a
track loads, decoded via bundled FFmpeg, and composited with the audio mix.

## Features

### v1 (shipped)

- **VideoDecoder**: Dedicated FFmpeg decode thread per deck (avcodec/avformat/swscale).
  Handles A/V sync (PTS-based), threaded decode, and seek.
- **VideoWidget**: QPainter-based rendering with aspect-ratio letterbox. Integrates into
  Mixxx's existing widget tree as a `<VideoWidget>` skin element.
- **ControlObjects**: `video_enabled` and `video_fullscreen` per deck.
- **Skin**: `<VideoWidget>` XML element added to LateNight deck layout.
- **Library**: `.mkv` and `.webm` added to format whitelist.
- **Build**: CMake + Ninja, 9.7 MB standalone exe.

### v2 (shipped)

- **VideoMixer**: Singleton compositing all deck video streams. Supports crossfader
  blending between decks — the video mix follows the audio crossfader.
- **VFX**: Per-deck video processing via ControlObjects: `video_brightness`,
  `video_contrast`, `video_saturation`. Real-time adjustment with 0.0–1.0 range.
- **VideoThumbnail**: FFmpeg keyframe extraction for waveform overviews. LRU cache
  (500 entries) keyed by track ID.
- **Hardware decode**: D3D11VA and CUDA via `av_hwdevice_ctx_create`. Falls back to
  software decode when no compatible GPU is found.
- **Video output panel**: Detachable fullscreen window for secondary monitor output
  (projector mode). Independent of the main Mixxx window.
- **Skin attributes**: `show_video_preview` (per-deck toggle to show/hide preview),
  `show_video_output` (toggle the output panel).
- **6 new ControlObjects**: `video_crossfader`, `video_brightness`, `video_contrast`,
  `video_saturation`, `video_enabled`, `video_fullscreen`.

## Architecture

### Video Module Structure

```
src/
└── video/
    ├── videodecoder.h/.cpp        # FFmpeg decode thread, A/V sync
    ├── videowidget.h/.cpp         # QPainter rendering widget
    ├── videomixer.h/.cpp          # Singleton compositor, crossfader blending
    ├── videothumbnail.h/.cpp      # Keyframe extraction, LRU cache
    ├── videoframe.h               # Shared decoded frame type (AVFrame wrapper)
    └── hw/
        ├── hwdevice_d3d11.cpp     # D3D11VA hardware decode
        └── hwdevice_cuda.cpp      # CUDA hardware decode
```

### ControlObjects

| CO | Deck | Range | Description |
|----|------|-------|-------------|
| `video_enabled` | Per-deck | 0/1 | Enable/disable video output for deck |
| `video_fullscreen` | Per-deck | 0/1 | Toggle fullscreen video window |
| `video_crossfader` | Global | 0.0–1.0 | Blend video between decks |
| `video_brightness` | Per-deck | 0.0–1.0 | Brightness adjustment |
| `video_contrast` | Per-deck | 0.0–1.0 | Contrast adjustment |
| `video_saturation` | Per-deck | 0.0–1.0 | Saturation adjustment |

### Companion Video Naming

For a track `artist - title.mp3`, the autoloader looks for (in order):
- `artist - title.mp4`
- `artist - title.mkv`
- `artist - title.mov`
- `artist - title.webm`

Same directory as the audio file. The `.mp4` format takes priority; if multiple
companion formats exist, the first found in the extension order above is used.

### OSC Integration

All video COs are addressable via OSC at `/deck/[N]/<co_name>` using the
companion mixx-dj-mcp server (ports 11118/11119). The full OSC address table:

```
/deck/[N]/video_enabled       → 1.0/0.0
/deck/[N]/video_fullscreen    → 1.0/0.0
/deck/[N]/video_brightness    → 0.0-1.0
/deck/[N]/video_contrast      → 0.0-1.0
/deck/[N]/video_saturation    → 0.0-1.0
/video_crossfader             → 0.0-1.0
```

## Build Instructions

### Prerequisites

- Visual Studio 2022 (MSVC v143)
- CMake 3.21+
- Ninja
- vcpkg (via `tools/windows_release_buildenv.bat`)
- FFmpeg 6.x+ dev libraries (avcodec, avformat, swscale, avutil, avdevice)

### Build Steps

```powershell
tools\windows_release_buildenv.bat
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="..\buildenv\mixxx-deps-2.5-x64-windows-release-40c29ff\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-release -G Ninja ..
ninja
```

Output: `build\mixxx.exe` (~9.7 MB).

### OSC Companion

The sister project [mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp)
provides AI-controlled OSC bridge for all video COs.

## Non-Goals

- No video recording or streaming (use OBS Studio)
- No video transitions/crossfading beyond the basic crossfader blend
- No real-time video effects apart from brightness/contrast/saturation
- No performance on systems without GPU hardware decode (software decode works but is CPU-heavy)
- No macOS or Linux support (Windows only — no plans to port)
- No video file import/re-encoding (use external tools)
