# NDI output

**Status:** **Partial MVP** — GPL-clean dynamic load (2026-07-27). **Not marked Works** until
OBS or NDI Studio Monitor confirms the feed with the NDI runtime installed.

**Licensing:** [`docs/NDI-LICENSING.md`](NDI-LICENSING.md) (required reading for contributors).

See `docs/TODO.md` item 27 and `docs/IDEAS.md` §3.

NDI is how mixxxxx sends **live video over the network** to OBS, Resolume, vMix, and
club media servers — the **video pipe** in the [AV orchestrator stack](ORCHESTRATOR.md).

---

## What is NDI?

**NDI®** (Network Device Interface) is a royalty-free protocol from [NDI/Vizrt](https://www.ndi.tv/)
for sending high-quality audio/video over a LAN. Think of it as:

> "This app appears as a **virtual camera / video source** on the network."

Other apps **subscribe** to that source by name. No file export, no window scraping — a proper
video pipe.

Common uses:

| Consumer | Why |
|---|---|
| **OBS Studio** | Stream with NDI Source plugin; mixxxxx video + OBS overlays |
| **Resolume Arena/Avenue** | Club VJ rig; mixxxxx mixes AV, Resolume adds layers |
| **vMix** | Broadcast switching |
| **Zoom / Teams** (with tools) | Sometimes via NDI bridges |

NDI is **not** a codec you play back from disk. It is a **live transport** between apps on
the same network (or NDI Bridge over WAN with extra setup).

---

## What mixxxxx does today

| Path | Status |
|---|---|
| Per-deck video decode + crossfader blend | Works (`VideoMixer`) |
| Fullscreen window on monitor 2 | Works (`VideoWidget`, CO) |
| **NDI® sender** | **Partial MVP** | `NdiOutput` + `NdiFrameUtil`; dynamic runtime load; CO `[Ndi],enabled`, `[Ndi],source_name`; `--ndi-enable` |

Today the video path is a **local window**. NDI publishes the same blended frame that
`VideoMixer::blendFrame()` already produces (QImage / RGB), on a timer aligned to video FPS.
Send runs on a **dedicated thread** with a bounded queue (drops frames if behind).

---

## Setup (runtime)

Mixxxxx does **not** ship the NDI runtime. Users install it separately:

1. Download and install the [NDI redistributable](https://ndi.link/NDIRedistV5) (v6 also works; see licensing doc).
2. Set environment variable **`NDI_RUNTIME_DIR_V5`** (or `NDI_RUNTIME_DIR_V6`) to the directory
   containing `Processing.NDI.Lib.x64.dll`.
3. Build Mixxxxx (NDI is **ON** by default when FFmpeg is enabled).
4. Enable `[Ndi],enabled` or pass `--ndi-enable`.
5. Open NDI Studio Monitor or OBS NDI Source and look for your source name (default `Mixxxxx`).

If the runtime is missing, Mixxxxx logs a warning and shows a dialog with the download link.

**Where the feed goes:** see [`NDI-TARGETS.md`](NDI-TARGETS.md) — OBS, Resolume, vMix, Studio Monitor, etc.

---

## Implemented

1. **CMake** `NDI=ON` (default) — requires `FFMPEG=ON`; **no** link to NDI libraries.
2. **MIT headers** in `lib/ndi/include/` (see `docs/NDI-LICENSING.md`).
3. **`NdiRuntime`** — `LoadLibrary` / `NDIlib_v5_load`; env-based discovery.
4. **`NdiOutput`** — 30 fps timer captures frames; **`NdiSenderWorker`** thread sends.
5. **`NdiFrameUtil`** — letterbox to 16:9 BGRA (`ndi_frame_util_test.cpp`).
6. **ControlObjects:** `[Ndi],enabled`, `[Ndi],source_name`.
7. **CLI:** `--ndi-enable`.
8. **`NDI=OFF`:** stub path; no NDI code compiled into send path.

**To verify:** install runtime, enable CO, confirm source in NDI Studio Monitor.

---

## What you need on the receiving side

1. Install **NDI Tools** (includes NDI Studio Monitor — free).
2. In OBS: add **NDI Source** (obs-ndi / DistroAV plugin) or use dedicated NDI input.
3. Same Wi‑Fi/LAN as the mixxxxx machine (gigabit wired preferred for 1080p).

---

## vs other options

| Approach | Pros | Cons |
|---|---|---|
| **Second monitor HDMI** | Simple, zero SDK | Cable length, no OBS without capture |
| **Window capture (OBS)** | No code in mixxxxx | Fragile, latency, scaling artifacts |
| **NDI** | Clean pipe, industry standard | Runtime install; LAN bandwidth |
| **Spout/Syphon** (Windows/macOS) | GPU texture share, low latency | Not cross-platform; separate feature |

---

## References

- [NDI targets — OBS, Resolume, vMix, …](NDI-TARGETS.md)
- [NDI licensing in Mixxxxx](NDI-LICENSING.md)
- [NDI SDK download](https://ndi.video/for-developers/ndi-sdk/)
- [NDI dynamic loading docs](https://docs.ndi.video/all/developing-with-ndi/sdk/dynamic-loading-of-ndi-libraries)
- [DistroAV (obs-ndi)](https://github.com/DistroAV/DistroAV)
- mixxxxx source: `src/video/ndi_runtime.{h,cpp}`, `src/video/ndioutput.{h,cpp}`, `src/video/ndi_frame_util.{h,cpp}`
