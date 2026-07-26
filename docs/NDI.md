# NDI output (planned)

**Status:** Not implemented. See `docs/TODO.md` item 27 and `docs/IDEAS.md` §3.

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
| **NDI sender** | **Absent** — no SDK, no sender in `src/` |

Today the video path is a **local window**. NDI would publish the same blended frame that
`VideoMixer::blendFrame()` already produces (QImage / RGB), on a timer aligned to video FPS.

---

## Planned design (when built)

1. **CMake option** `NDI=ON` (default **OFF**) — like `FFMPEG`, `ONNX_RUNTIME`.
2. **NDI Advanced SDK** (free download from ndi.video; license acceptance required).
3. **`NdiOutput` class** — create sender, convert QImage → NDI frame, send each blended frame.
4. **ControlObjects** (names TBD), e.g.:
   - `[Ndi],enabled`
   - `[Ndi],source_name` (string CO or config key)
5. **CLI flags** (future): `--ndi-enable`, `--ndi-name Mixxxxx`
6. **Unit test** — frame dimensions / pixel layout conversion only (no network in CI).

We will **not** mark NDI "Works" until a receiver (OBS or NDI Studio Monitor) shows the feed.

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

NDI is the feature that makes mixxxxx a **node in a pro rig**. Fallback chain is the feature
that makes video **usable on a normal library**. Pick order based on whether you care more
about **club/stream plumbing** or **daily mixing with video**.

---

## References

- [NDI SDK download](https://ndi.video/for-developers/ndi-sdk/)
- [OBS NDI plugin](https://github.com/obs-ndi/obs-ndi)
- mixxxxx source (future): `src/video/ndioutput.{h,cpp}` (not created yet)
