# Mixxxxx Feature Proposals

Written 2026-07-26 after the source audit. Nothing here is implemented.

Two framings worth keeping in mind while reading:

**Where this project is fast, and where it is not.** The A/V sync work took three
sprints because real-time C++ media-engine code is genuinely hard. The exporter
classes landed in a day because they are library code. Pick fights on the layer
where velocity is high, which is the AI and integration layer, not the media engine.

**Serato's moat is hardware certification and twenty years of controller mappings.**
Not worth attacking head-on. Their weakness is a conservative release cadence,
because their business is selling stability to working DJs mid-gig. That is the same
reason they have declined to add video. The edge here is velocity on things they
structurally will not ship.

---

## 1. Beat-locked video FX

**Build this first.** The expensive infrastructure already exists and is already
correct.

`VideoSyncControl` publishes sample-accurate deck position on every engine buffer.
Mixxx's analyzer stack (`analyzerbeats`) already computes a beat grid for every
library track. Wire those together and video cuts, strobes, zoom pumps, RGB splits,
and feedback trails land exactly on the beat, and more importantly on the 16 and 32
bar phrase boundary.

Resolume does this and charges for it, and it still has to be mapped by hand per
track. Nobody does it automatically off an analyzed library.

Sketch: a `VideoFxChain` reading `[ChannelN],beat_distance` and `beat_closest`,
driving parameterised effects applied in `VideoWidget::paintEvent` or, better, in
`VideoMixer` so effects survive into the master output. Effects as small composable
units with a beat-division parameter (every beat, every 4, every 16).

Why it is cheap: the hard part, a frame-accurate audio clock, is the part that
already works.

Est: 3 to 4 days.

---

## 2. Video fallback chain

**This is the actual adoption blocker for video DJing and everyone quietly ignores
it.** Roughly 95% of any real library has no companion video file. Today that means
a black rectangle, which means the feature goes unused.

Per deck, walk a chain until something produces frames:

1. Companion file (current behaviour)
2. A beat-matched loop pulled from a tagged pool, selected by genre or energy
3. Generative visuals (Butter Churn and MilkDrop are already in mixx-dj-mcp)
4. Album art with slow Ken Burns motion

The sub-feature that makes it sing: **time-stretch the pool loop to the deck BPM**,
so a 128 BPM visual loop plays correctly under a 174 BPM track. You have the deck
rate and the loop's own duration, so this is a playback rate multiplier, not real
DSP work.

Selection of which loop to pull is where the AI layer earns its place: match on
energy and genre from data the analyzer already extracted.

Est: 4 to 5 days including BPM-matched loop playback.

---

## 3. NDI output

**This is what gets mixxxxx into actual club and stream rigs.**

Video output today is a fullscreen window on a second monitor, which is a closed
box. Publish the mixed frame as an NDI source and mixxxxx becomes a video *source*
that Resolume, OBS, vMix, and most club media servers can pull over the network.

The NDI SDK is free and the API is close to "create sender, push frame". The frame
is already sitting in `VideoMixer::blendFrame()` as a QImage, so this is largely a
format conversion and a send call on a timer.

Strategically this reframes the project from "Mixxx with a video window" to "the
free video DJ front end that plugs into the rig you already have". For a working VJ
that is the difference between a curiosity and a tool.

Est: 2 to 3 days.

---

## Honourable mentions

**Video hot cues.** Audio and video jump together. Nearly free now that
`VideoSyncControl` works, because the seek path already exists.
Est: 1 day.

**Stem-driven visuals.** Map vocal stem energy to one visual layer, drums to
another. Genuinely unprecedented; even Algoriddim does not drive video off stems.
Blocked on ONNX, which is currently `OFF` in the build, so
`src/stems/stem_separator.cpp` has never been compiled here and its real state is
unknown.

---

## Infrastructure proposal A: the OSC server

There is no OSC code anywhere in `src/`. A grep of 1887 files for `QUdpSocket`,
`OscServer`, `11118`, and `11119` returns zero hits. Everything the README documents
about OSC, and every deck and mixer tool in mixx-dj-mcp, depends on a component that
does not exist.

**No new dependency needed.** OSC 1.0 wire format is a null-terminated, 4-byte-padded
address string, a comma-prefixed type tag string, then 4-byte-aligned arguments.
Hand-rolling encode and decode is under 150 lines. liblo is not worth the vcpkg
friction.

Design:

- `src/control/oscserver.{h,cpp}`, `QUdpSocket` bound to 11119
- Generic address to ConfigKey translation: `/deck/N/xxx` becomes
  `ConfigKey("[ChannelN]", "xxx")`, `/export/...` becomes `[Export]`. This covers
  most of the surface without per-control code.
- Inbound calls `ControlObject::set()` directly. ControlObjects are thread-safe, so
  no marshalling to the GUI thread is required.
- Outbound: `valueChanged` connections on a **configurable subscribed set**, encoded
  and sent to 11118. Do not blanket-subscribe; Mixxx has thousands of COs and
  flooding UDP at engine rate will hurt.
- Instantiate from `CoreServices` next to `registerExportControls()`.
- First pass reads enable, host, and ports from config keys. A preferences page is a
  second step.

On the mixx-dj-mcp side, two prerequisites:

- Remove `autouse=True` from `auto_mock_bridge` in `tests/conftest.py`, or any new
  integration test gets mocked out like everything else.
- `OscBridge.is_connected()` returns `self._running`, which only reports whether its
  own thread started. UDP send never raises against a dead port, which is why every
  tool currently reports success while doing nothing. Replace with a heartbeat query
  and a timeout once there is something to answer it.

Est: 1 day bidirectional, plus half a day for a preferences page.

---

## Infrastructure proposal B: extend the CLI

Underrated, and cheaper than OSC for most of what OSC is currently wanted for.

With no OSC and no scripting surface, the command line is the only automation entry
point into mixxxxx, and it is thin: fullscreen, locale, settings path, resource path,
developer mode, log level, music files.

Proposed additions, roughly by value:

### `--set-control "[Group],name=value"` (repeatable)
Apply ControlObject values after startup. This single flag replaces most of what OSC
is wanted for during development and testing, and makes the video and export features
scriptable today rather than after the OSC server lands.

```
mixxx.exe --set-control "[Skin],show_video_output=1" ^
          --set-control "[Channel1],video_enabled=1" ^
          --set-control "[Mixer],crossfader=0.5"
```

### `--dump-controls`
Print every registered ControlObject with group, name, and current value, then exit.
Self-documenting control surface. This would have made the entire 2026-07-26 audit
trivial: every dead CO found by hand (`video_saturation`, `video_crossfader`,
`setSpeed`) would have been visible in a diff of two dumps.

### `--export-crate <name> --export-format <engine|serato|virtualdj> --export-path <dir>`
Run the export, then exit. **This dissolves TODO item 12 entirely.** The crate
selection problem exists because ControlObjects hold doubles only, so the
`[Export],rekordbox_usb_path` string CO documented in the README is not
implementable as described. A CLI flag has no such limitation, is scriptable, is
testable without a GUI, and also fixes the unsafe `defaultExportPath()` heuristic by
making the destination explicit.

### `--video-screen <n>`
Choose which monitor receives fullscreen video output. Currently auto-detected, which
is exactly the kind of thing that goes wrong at a venue five minutes before doors.

### `--load-video <deck> <path>`
Override companion-file resolution for a specific deck. Useful for testing and for
the case where the video lives somewhere other than next to the audio.

### `--osc-port <in> <out>`
Once the OSC server exists.

Est: 1 day for the set above. `--set-control` and `--dump-controls` alone are worth
half of it.

---

## Suggested order

1. CLI `--set-control` and `--dump-controls`. Cheapest, and it makes everything else
   easier to test.
2. Beat-locked video FX. Highest visible payoff per hour, infrastructure already
   there.
3. CLI `--export-crate`, which closes the exporter story.
4. OSC server, which makes mixx-dj-mcp real.
5. NDI output.
6. Video fallback chain.
