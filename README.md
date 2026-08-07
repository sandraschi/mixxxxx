# Mixxxxx — Video-Enabled Mixxx Fork

![Mixxx](https://img.shields.io/badge/base-Mixxx_2.5.6-orange)
![Video](https://img.shields.io/badge/video-FFmpeg-blue)
![Build](https://img.shields.io/badge/build-CMake%2FNinja-green)
![License](https://img.shields.io/badge/license-GPLv2-blue)

**Mixxxxx** extends [Mixxx](https://mixxx.org/) with video and fleet integration. In this stack it acts as the **AV orchestrator hub**: mix audio and deck video locally, publish the blend via **NDI®** to Resolume/OBS, and accept automation over **OSC** from [mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp).

See [`docs/ORCHESTRATOR.md`](docs/ORCHESTRATOR.md) for the full rig diagram (control pipe vs video pipe).

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
- **Hardware decode**: D3D11VA + CUDA attempted via `av_hwdevice_ctx_create` — **not active** in current build (no `get_format` callback; software decode only). See `docs/STATUS.md`.
- **VideoThumbnail**: FFmpeg keyframe extraction, 500-entry LRU cache
- **Video output panel**: Detachable fullscreen + projector output

### Sprint 1 (v3)
- **Phase indicator**: `[Channel{N}],phase` CO (0-360°), QPainter arc ring widget with green→yellow→orange→red gradient, `<PhaseIndicator>` skin element in LateNight
- **Rekordbox export**: `RekordboxExporter` reads Mixxx SQLite, writes Pioneer .pdb via libdjinterop; `[Export],rekordbox_usb_path`, `[Export],export_crate`, `[Channel{N}],export_rekordbox` COs

### NDI® network video (optional)

Mixxxxx can publish the crossfader-blended video mix as an **NDI®** source on your LAN
(`[Ndi],enabled`, `--ndi-enable`). This is **GPL-clean**: we ship MIT SDK headers only and
load the proprietary runtime dynamically — we do **not** bundle NDI.

1. Install the free [NDI redistributable](https://ndi.link/NDIRedistV5) (or v6 runtime).
2. Set `NDI_RUNTIME_DIR_V5` to the folder containing `Processing.NDI.Lib.x64.dll`
   (e.g. `C:\Program Files\NDI\NDI 5 Runtime\v5`).
3. Enable NDI in Mixxxxx; receivers (OBS NDI Source, NDI Studio Monitor) subscribe by source name.

Licensing details: [`docs/NDI-LICENSING.md`](docs/NDI-LICENSING.md). User guide: [`docs/NDI.md`](docs/NDI.md).

Build with `-DNDI=OFF` to omit NDI send support entirely (default **ON** when FFmpeg is enabled).


> **Status 2026-07-26:** OSC server **MVP shipped** in `src/control/oscserver.cpp`.
> UDP **11119** in, **11118** out; heartbeat `/mixxxxx/ping` → `/mixxxxx/pong`.
> Full truth table: [`docs/STATUS.md`](docs/STATUS.md). Preferences UI for OSC still TODO.

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
| `/deck/[N]/phase` | 0–360 | Beat phase alignment (degrees) |
| `/mixxxxx/ping` | — | Bridge heartbeat (returns `/mixxxxx/pong`) |

`/video_crossfader` and export string COs from early specs are **not** implemented as
documented here — video follows the audio crossfader; crate export use `--export-crate`
CLI. See `docs/TODO.md` items 12–13, 23.

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

## Documentation

| Topic | File |
|---|---|
| What works (source of truth) | [`docs/STATUS.md`](docs/STATUS.md) |
| Session notes (NDI, skins, decisions) | [`docs/NOTES-20260726.md`](docs/NOTES-20260726.md) |
| Skins (no marketplace — install guide) | [`docs/SKINS.md`](docs/SKINS.md) |
| **AV orchestrator** (mixxxxx hub, NDI, Resolume, OBS) | [`docs/ORCHESTRATOR.md`](docs/ORCHESTRATOR.md) |
| **NDI® output** | [`docs/NDI.md`](docs/NDI.md) · [`docs/NDI-TARGETS.md`](docs/NDI-TARGETS.md) · [`docs/NDI-LICENSING.md`](docs/NDI-LICENSING.md) |
| Video feature roadmap | [`docs/IDEAS.md`](docs/IDEAS.md) |
| Session notes | [`docs/NOTES-20260726.md`](docs/NOTES-20260726.md) |
| Progress log | [`docs/PROGRESS-20260726.md`](docs/PROGRESS-20260726.md) |
| Rane hardware handoff | [`docs/HANDOFF-RANE-MAPPING.md`](docs/HANDOFF-RANE-MAPPING.md) |

## Credits

Mixxxxx is a fork of [Mixxx](https://mixxx.org/) — GPLv2 licensed.
All Mixxx upstream credits apply. The video module was added as a custom
extension; no upstream patches have been submitted.

- [Mixxx GitHub](https://github.com/mixxxdj/mixxx)
- [FFmpeg](https://ffmpeg.org/) — video decoding library (not bundled in upstream Mixxx)
- [mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp) — companion OSC control server

## License

GNU General Public License v2 (same as Mixxx).
