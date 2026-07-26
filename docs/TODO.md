# Mixxxxx TODO

Ordered by value per hour. Estimates assume AI-assisted work on a codebase already
loaded in context.

Evidence for every item is in `docs/ASSESSMENT-20260726.md`, section numbers cited.
Session results are in `docs/PROGRESS-20260726.md`. Feature proposals are in
`docs/IDEAS.md`.

---

## DONE (2026-07-26 fix pass, built and tested)

| # | Item | Assessment ref |
|---|---|---|
| 1 | VideoMixer keyed on deck number, not FFmpeg stream index | 3.1 |
| 2 | Blend curve corrected to `(xfader+1)/2` | 3.2 |
| 3 | `videomixer_test.cpp` written, 7 cases, registered under `if(FFMPEG)` | 5 |
| 5 | Scaler built from actual frame format via `sws_getCachedContext` | 3.4 |
| 6 | `QWaitCondition` now holds its mutex, pause is no longer UB | 3.7 |
| 8 | `applySaturation` wired into `paintEvent` | 3.3 |
| 11 | Per-deck export resolves the real track path via `PlayerManager` | 3.9 |
| 14 | Hot cue filter uses `CueType::HotCue` instead of literal 2 | 3.11 |
| - | Deep copy before compositing, no frame burn-in | 7.1 area |
| - | Dead `[Mixer],video_crossfader` CO removed, unblocked the test suite | 7.1 |
| - | `mkv`/`webm` excluded in `taglibStringToEnumFileType`, suite back to green | 7.2 |
| - | **OSC server MVP** (UDP 11119/11118, `/mixxxxx/ping`) | 4, 18 |
| - | **CLI** `--set-control`, `--dump-controls`, `--gig-script`, crate export/import | 24 |
| - | **`docs/NDI.md`** primer (planned network video) | 27 doc |
| - | **`docs/SKINS.md`** + **MixxxxxVideo** skin (Daylight scheme) | — |
| - | README documentation table | 21 partial |
| - | **mixx-dj-mcp Help page** (`/help`, NDI tab) | 33 |
| - | **Beat-locked video FX MVP** (strobe, zoom, VideoFxChain) | 25 partial |

---

## P0: verify the core feature by eye

### 4. Gig-test the video mix
The unit tests prove the mixer composites correctly. They do not prove it looks
right in motion. Two decks, two companion videos, `[Skin],show_video_output` = 1,
sweep the crossfader, watch the `[Master]` panel.

Also worth checking here: the 2 second hard-seek threshold in the A/V sync drift
logic. Fast repeated-direction scratching will likely oscillate between the skip
and throttle tiers rather than scrub smoothly, and `av_seek_frame` only lands on
keyframes. Functionally synced is not the same as frame-accurate under scratching.
Est: one evening with real tracks.

---

## P1: remaining correctness

### 7. Decide on hardware decode: fix it or drop the claim
Assessment 3.5. `initHardwareDecoder()` assigns `hw_device_ctx` but installs no
`get_format` callback, so hwaccel never engages and decode is silently software.
Now that the scaler handles NV12 correctly (item 5), actually enabling hwaccel is
safe to attempt.
Est: 3 hours to fix properly, 10 min to drop the claim honestly.

### 9. Fix or remove `setSpeed`
Assessment 3.3. Dead method, never called. Either drive it from the deck rate CO or
delete it.
Est: 15 min.

### 10. Stop duplicating COs in the fullscreen window
Assessment 3.8. `slotVideoFullscreen` constructs a second `VideoWidget` on the same
group, re-registering five ControlObjects on existing keys. Add a render-only mode,
or split out a lightweight frame-display widget that registers nothing.
Est: 1 hour.

---

## P2: exporters

### 12. Crate export needs a selection mechanism
Assessment 3.9. Currently refuses loudly rather than querying for a crate named
`""`. ControlObjects hold doubles only, so the `[Export],rekordbox_usb_path` string
CO documented in the README is not implementable as described.

**Preferred solution: do this via the CLI (item 24), not a dialog.** A
`--export-crate <name>` flag sidesteps the CO type limitation entirely, is
scriptable, and is testable without a GUI. A preferences dialog can come later if
anyone wants it interactively.
Est: folded into item 24.

### 13. Rename the Rekordbox exporter to what it is
Assessment 3.10. libdjinterop writes Engine Library, not Pioneer DeviceSQL. Rename
`RekordboxExporter` to `EngineExporter`, change the target directory from
`PIONEER/rekordbox` to `Engine Library/`, update COs and docs. The feature becomes
correct and genuinely useful instead of silently producing a USB stick no CDJ will
read. Free Engine export is not widely available.
Est: 2 hours plus doc updates.

### 15. Do not recreate the database per track
Assessment 3.12. `exportCrate` loops `exportTrack`, and `exportTrack` calls
`create_database` each time. Open once, loop `create_track`.
Est: 1 hour.

### 16. Make the export destination explicit
Assessment 3.13. `defaultExportPath()` picks the first ready non-NTFS volume under
128 GB, which can select an SD card, a mounted ISO, or a FAT32 EFI partition, then
`mkpath`s into it. Replace with an explicit path, ideally the same CLI flag as
item 24.
Est: 1 hour.

### 17. Write `exporter_test.cpp`
Seed a temp SQLite with the Mixxx schema, export one track, assert track count and
hot cues. Locks in items 11, 14, 15.
Est: 2 hours.

---

## P3: OSC server

### 18. Implement an OSC server in mixxxxx
Assessment 4. **MVP implemented 2026-07-26:** `src/control/oscserver.cpp`, ports
11119/11118, deck/crossfader/effect mapping, outbound deck CO subscriptions,
`/mixxxxx/ping` → `/mixxxxx/pong`. Preferences page still TODO.

### 19. Fix the mixx-dj-mcp test blindness
`tests/conftest.py` declares `auto_mock_bridge` with `autouse=True`, so no test can
observe a dead bridge. Make it opt-in, add an integration test that binds a real UDP
socket on 11119. It fails today, correctly, and goes green when item 18 lands.
Est: 1 hour.

### 20. Make `is_connected()` mean something
`osc_bridge.py` returns `self._running`, only whether its own thread started. Add a
heartbeat query with a timeout. Depends on item 18.
Est: 2 hours.

---

## P4: CLI

### 31. Rane Seventy-Two MKII controller mapping
Not supported today: zero Rane mappings in `res/controllers`, none upstream.
Buildable. Research in `docs/RANE-SEVENTY-TWO-MKII.md`, agent-ready task brief in
`docs/HANDOFF-RANE-MAPPING.md`.

Do DVS first (half a day, no mapping needed, `VINYLCONTROL=ON` already), MIDI
mapping second. FX section is blocked by firmware and should be documented as a gap
rather than solved. Touchscreen is out of scope.

Note: `--dump-controls` (item 24) makes MIDI mapping jobs easier; implemented 2026-07-26.
Est: 1 day for a solid faders/EQ/pads mapping, 1 to 2 more for LED feedback.

### 32. Shelf Hercules DJ Console (Mk1/Mk2, ~2003)
Maps already in `res/controllers/` — plug in and smoke-test when motivated; no new mapping work unless hardware fails to match a preset.

### 24. Extend the command line
Full rationale in `docs/IDEAS.md`. CLI and OSC MVP are in place; remaining flags below. Highest-value flags:

- `--set-control "[Group],name=value"` (repeatable), applied after startup **Implemented 2026-07-26**
- `--dump-controls` then exit, listing every registered CO **Implemented 2026-07-26**
- `--gig-script <path>` line-based gig setup (set/load/video/queue-crate/autodj) **Implemented 2026-07-26**
- `--no-banner` suppress stderr + GUI startup banner **Implemented 2026-07-26**; GUI welcome dialog with “Show this welcome on startup” checkbox (`[Config],startup_banner_show`) **2026-07-27**
- `--export-crate <name> --export-format <engine|serato|virtualdj> --export-path <dir>` then exit **Implemented 2026-07-26**
- `--import-crate <path> [--into-crate <name>]` (M3U/PLS/CSV, `.vdjfolder`, Serato `.crate`) **Implemented 2026-07-26**
- `--video-screen <n>` to pick the fullscreen output monitor
- `--load-video <deck> <path>` to override companion resolution

Example gig script: `docs/example-gig.mixxx`

Community skins: `docs/SKINS.md` (no VDJ-style marketplace — forum/GitHub manual install).

---

## P5: new functionality

See `docs/IDEAS.md` for full write-ups.

### 25. Beat-locked video FX
Highest leverage, because the expensive infrastructure already works.
**MVP 2026-07-26:** `VideoFxChain` — strobe + zoom pump, beat division CO,
applied in `VideoMixer::blendFrame()` (master output). `videofxchain_test.cpp` (8 cases).
Remaining: cut, RGB split, feedback trail; skin UI buttons; phrase-aware 16/32 bars via beat grid.
Est: 1–2 days for remaining effects + UI.

### 26. Video fallback chain
Solves the actual adoption blocker: most libraries have no companion video.

**Partial 2026-07-27 (step 4):** `VideoFallback::renderKenBurns()` — album art
slow zoom/pan when no companion video; wired in `VideoWidget` at 30fps;
CO `[ChannelN],video_fallback` (default 1). `videofallback_test.cpp` (3 cases).

**Partial 2026-07-27 (step 2):** `VideoPool` — tagged loop directory
(`%MIXXX_SETTINGS%/video-pool` or `--video-pool`). Filename `{bpm}-{genre}-{energy}.mp4`
or sidecar `.json`. Selection by BPM/genre/energy; playback via `VideoDecoder`
pool sync mode (`deckBpm/loopBpm * rate_ratio`). `videopool_test.cpp` (5 cases).

**Partial 2026-07-27 (step 3):** `VideoGenerative` — beat-reactive pulses/bars
(`beat_distance`, `bpm`, deck volume); chain step between pool and Ken Burns;
CO `[ChannelN],video_fallback_generative` (default 1). Native QImage MVP — Butter
Churn remains in mixx-dj-mcp webapp. `videogenerative_test.cpp` (4 cases).

Fallback chain **complete** (steps 2–4). NDI MVP landed (27, partial — SDK verify pending).

### 27. NDI output
Turns the project from a closed box into a video source for real rigs.

**Partial 2026-07-27:** `NdiOutput` publishes `VideoMixer::blendFrame()` at ~30 fps.
COs `[Ndi],enabled`, `[Ndi],source_name`; CLI `--ndi-enable`. CMake `NDI=ON` +
`NDI_SDK_DIR` links NDI SDK; without SDK the stub logs and sends nothing.
`NdiFrameUtil` letterbox + 3 unit tests. **Not Works** until OBS / NDI Studio Monitor
confirms the feed. Primer: [`docs/NDI.md`](docs/NDI.md).

### 28. VJ integration: OSC-out + Spout (Windows)
Local interoperability with Magic, TouchDesigner, Synesthesia, etc. without buying Resolume.
**Not implemented** — spec: [`docs/vj-integration-spout-osc.md`](vj-integration-spout-osc.md)
(Opus research note 2026-07-27). **Sequence after NDI (27)** unless a same-PC VJ rig becomes
urgent before OBS/Kick wiring.

| Phase | Work | Est |
|---|---|---|
| A | **OSC-out for VJ** — extend `OscServer` with `/mixxxx/deck/N/*` schema (beat_distance, bpm, play, crossfader); 50 Hz timer; event-driven track metadata. Do **not** add a second UDP server. | ½ day |
| B | **Spout master sender** — `spoutDX` from `VideoMixer::blendFrame()` (CPU `QImage` → DX11); dedicated sender thread; `#ifdef WIN32` only | 1–2 days |
| C | Preferences + COs `[VJ],spout_enabled`, `[VJ],osc_enabled` | ½ day |
| D | Per-deck Spout senders / Spout receiver — only after gig-testing A–C | later |

Key spec finding: pipeline is CPU `QImage` end-to-end → use **SpoutDX**, not SpoutGL.
NDI = network (OBS stream); Spout = same-machine GPU handoff. Different jobs.

### 29. Video hot cues
Nearly free now that sync works.
Est: 1 day.

### 30. Stem-driven visuals
Blocked on ONNX. Genuinely unprecedented if it lands.
Est: unknown, gated on item 31.

### 31. Turn `ONNX_RUNTIME` on and see what happens
`src/stems/stem_separator.cpp` exists but has never been compiled in this build
config. Its actual state is unknown and unassessed.
Est: half a day to find out.

## P6: documentation debt

### 21. Rewrite README.md and PRD.md against the code
Both still document hardware decode and Pioneer pdb output as working in the feature
bullets and PRD body. The OSC section in README still carries a "NOT IMPLEMENTED"
banner from before item 18 landed — **remove or replace that banner**; OSC MVP is in
`src/control/oscserver.cpp`. Warning banners are in place on PRD but bodies are unchanged.

Partial 2026-07-26: README doc table (`STATUS`, `SKINS`, `NDI`, `IDEAS`, Rane handoff).

Rule going forward: a feature is documented as working only after a test asserts it,
or after a traced call-site verification recorded in STATUS.md.
Est: 2 hours (README feature bullets + PRD remain).

### 22. Add a `--developer` smoke checklist to docs
Developer Tools CO browser plus `--dump-controls` / `--set-control` cover video and export; write a short checklist anyway.
Est: 30 min.

### 23. Independent video crossfader
The `[Mixer],video_crossfader` CO was dead and has been removed, so video currently
follows the audio crossfader. An independent one with a link-to-audio toggle
(default linked) is a real feature a video DJ would want.
Est: 3 hours.
