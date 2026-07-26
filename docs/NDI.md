# NDI output

**Status:** **Partial MVP** (2026-07-27) — code in tree; builds without NDI SDK as a stub.
**Not marked Works** until OBS or NDI Studio Monitor shows the feed with `-DNDI=ON`.

See `docs/TODO.md` item 27 and `docs/IDEAS.md` §3.

NDI is how mixxxxx would send **live video over the network** to OBS, Resolume, vMix, and
club media servers — without a second monitor cable or HDMI capture card.

---

## What is NDI?

**NDI** (Network Device Interface) is a royalty-free protocol from [NDI/Vizrt](https://www.ndi.tv/)
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
| **NDI sender** | **Partial MVP** | `NdiOutput` + `NdiFrameUtil`; CO `[Ndi],enabled`, `[Ndi],source_name`; `--ndi-enable`; CMake `NDI=ON` + SDK for live send |

Today the video path is a **local window**. NDI would publish the same blended frame that
`VideoMixer::blendFrame()` already produces (QImage / RGB), on a timer aligned to video FPS.

---

## Implemented (2026-07-27)

1. **CMake option** `NDI=ON` (default **OFF**) — requires `FFMPEG=ON`.
2. **`NDI_SDK_DIR`** or auto-detect under `C:/Program Files/NDI/NDI {5,6} SDK`.
3. **`NdiOutput`** (`src/video/ndioutput.{h,cpp}`) — 30 fps timer, reads `VideoMixer::blendFrame()`.
4. **`NdiFrameUtil`** — letterbox to 16:9 ARGB32/BGRA for NDI (`ndi_frame_util_test.cpp`, 3 cases).
5. **ControlObjects:**
   - `[Ndi],enabled` (push button, default off)
   - `[Ndi],source_name` (UserSettings string, default `"Mixxxxx"`)
6. **CLI:** `--ndi-enable` sets `[Ndi],enabled` at startup.
7. **Stub path:** without SDK, enabling logs a warning; no network traffic.

**To verify:** install NDI SDK, rebuild with `-DNDI=ON -DNDI_SDK_DIR=...`, enable CO,
open NDI Studio Monitor or OBS NDI Source.

---

## Original plan (reference)

---

## What you need on the receiving side

1. Install **NDI Tools** (includes NDI Studio Monitor — free).
2. In OBS: add **NDI Source** (obs-ndi plugin) or use dedicated NDI input.
3. Same Wi‑Fi/LAN as the mixxxxx machine (gigabit wired preferred for 1080p).

---

## vs other options

| Approach | Pros | Cons |
|---|---|---|
| **Second monitor HDMI** | Simple, zero SDK | Cable length, no OBS without capture |
| **Window capture (OBS)** | No code in mixxxxx | Fragile, latency, scaling artifacts |
| **NDI** | Clean pipe, industry standard | SDK + firewall; LAN bandwidth |
| **Spout/Syphon** (Windows/macOS) | GPU texture share, low latency | Not cross-platform; not in plan yet |

---

## Recommended build order (fleet)

From `docs/IDEAS.md` / `docs/CURSOR-PROMPT-FEATURES.md`:

1. Beat-locked video FX (uses existing beat COs)
2. Video fallback chain (most libraries have no companion video)
3. **NDI output** ← this doc

**Fleet context:** Dani's Sunday Kick broadcast runs through **OBS**; **obs-mcp** is in the
fleet for scene/source automation. NDI sender + obs-mcp is the intended Sunday-evening
stack once items 1–2 land — mixxxxx → NDI → OBS → Kick, with MCP on both sides.

NDI is the feature that makes mixxxxx a **node in a pro rig**. Fallback chain is the feature
that makes video **usable on a normal library**. Pick order based on whether you care more
about **club/stream plumbing** or **daily mixing with video**.

---

## References

- [NDI SDK download](https://ndi.video/for-developers/ndi-sdk/)
- [OBS NDI plugin](https://github.com/obs-ndi/obs-ndi)
- mixxxxx source: `src/video/ndioutput.{h,cpp}`, `src/video/ndi_frame_util.{h,cpp}`
