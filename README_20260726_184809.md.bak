# Mixxxxx — Video-Enabled Mixxx Fork

![Mixxx](https://img.shields.io/badge/base-Mixxx_2.5.6-orange)
![Video](https://img.shields.io/badge/video-FFmpeg-blue)
![Build](https://img.shields.io/badge/build-CMake%2FNinja-green)
![License](https://img.shields.io/badge/license-GPLv2-blue)

**Mixxxxx** extends [Mixxx](https://mixxx.org/) — the leading open-source DJ software —
with real-time video playback via bundled FFmpeg. Load a track, get its companion video
automatically decoded and mixed alongside the audio.

Companion MCP server: [mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp) (AI-powered OSC control).

## Quick Build

```powershell
tools\windows_release_buildenv.bat
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="..\buildenv\mixxx-deps-2.5-x64-windows-release-40c29ff\scripts\buildsystems\vcpkg.cmake" -DVCPKG_TARGET_TRIPLET=x64-windows-release -G Ninja ..
ninja
```

Output: `build\mixxx.exe` (~9.7 MB).

## Features

### v1
- FFmpeg video decode (avcodec/avformat/swscale) per deck
- Companion video autoload: `.mp4` / `.mkv` / `.mov` / `.webm`
- QPainter rendering with aspect-ratio letterbox
- Fullscreen output to secondary monitor
- `<VideoWidget>` skin element in LateNight/Deere

### v2
- **VideoMixer**: Singleton compositor with crossfader blending
- **VFX**: Per-deck brightness/contrast/saturation COs
- **Hardware decode**: D3D11VA + CUDA via `av_hwdevice_ctx_create`
- **VideoThumbnail**: FFmpeg keyframe extraction, 500-entry LRU cache
- **Video output panel**: Detachable fullscreen + projector output

### Sprint 1 (v3)
- **Phase indicator**: `[Channel{N}],phase` CO (0-360°), QPainter arc ring widget with green→yellow→orange→red gradient, `<PhaseIndicator>` skin element in LateNight
- **Rekordbox export**: `RekordboxExporter` reads Mixxx SQLite, writes Pioneer .pdb via libdjinterop; `[Export],rekordbox_usb_path`, `[Export],export_crate`, `[Channel{N}],export_rekordbox` COs

## OSC Control Table

Control Mixxxxx via OSC (using [mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp)
or any OSC client). Configure Mixxx Preferences → MIDI/OSC:
- Output port: **11118** (Mixxx sends status here)
- Input port: **11119** (commands arrive here)

| OSC Address | Range | Description |
|-------------|-------|-------------|
| `/deck/[N]/video_enabled` | 0/1 | Toggle video on/off for deck |
| `/deck/[N]/video_fullscreen` | 0/1 | Toggle fullscreen video window |
| `/deck/[N]/video_brightness` | 0.0–1.0 | Adjust brightness |
| `/deck/[N]/video_contrast` | 0.0–1.0 | Adjust contrast |
| `/deck/[N]/video_saturation` | 0.0–1.0 | Adjust saturation |
| `/video_crossfader` | 0.0–1.0 | Blend video between decks |
| `/deck/[N]/phase` | 0–360 | Beat phase alignment (degrees) |
| `/deck/[N]/export_rekordbox` | 0/1 | Trigger Rekordbox export for deck's track |
| `/export/rekordbox_usb_path` | string | Set USB path for Pioneer export |
| `/export/export_crate` | 0/1 | Trigger crate export to Rekordbox format |

All video COs match Mixxx's native ControlObject addresses. Export COs are registered
by `registerExportControls()` in `src/export/export_controls.cpp`.

## Companion MCP Server

[mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp) is a FastMCP 3.4+ server that
bridges AI assistants (Claude Desktop, Cursor, opencode) to Mixxxxx/Mixxx via OSC:

```bash
uv sync
uv run uvicorn mixx_dj_mcp.server:app --port 11116 --reload
```

Video control commands: `video_enable`, `video_fullscreen` — available as `mixx_deck`
operations. See the [mixx-dj-mcp README](https://github.com/sandraschi/mixx-dj-mcp)
for full tool reference and webapp documentation.

## Usage Guide

### Video Container Format

Supported formats: **MP4**, **MKV**, **MOV**, **WebM**.

Codec recommendations:
- **Video**: H.264 (best compatibility), H.265/HEVC (GPU decode), VP9 (WebM)
- **Audio**: AAC, MP3, PCM

External tools for batch conversion: FFmpeg, HandBrake, Shutter Encoder.

### Companion File Naming

Place a video file with the **same basename** as your audio track in the **same directory**:

```
Music/
├── artist - title.mp3        ← audio track
└── artist - title.mp4        ← autoloaded companion video
```

Priority order (first match wins): `.mp4` > `.mkv` > `.mov` > `.webm`.

### Enabling Video in the Skin

1. Select a skin that contains a `<VideoWidget>` element (LateNight or Deere).
2. The video preview appears in the deck column when a companion file exists.
3. Use the `video_enabled` toggle (CO or OSC) to show/hide the preview.
4. Use `video_fullscreen` to send video to a secondary monitor/projector.

### Keyboard Shortcuts (if mapped)

Default OSC bindings from mixx-dj-mcp provide `video_enable` and `video_fullscreen`
commands. Custom MIDI/OSC mappings can be created in Mixxx Preferences → MIDI/OSC.

## Credits

Mixxxxx is a fork of [Mixxx](https://mixxx.org/) — GPLv2 licensed.
All Mixxx upstream credits apply. The video module was added as a custom
extension; no upstream patches have been submitted.

- [Mixxx GitHub](https://github.com/mixxxdj/mixxx)
- [FFmpeg](https://ffmpeg.org/) — video decoding library (not bundled in upstream Mixxx)
- [mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp) — companion OSC control server

## License

GNU General Public License v2 (same as Mixxx).
