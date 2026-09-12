# Mixxxxx Status

Last verified against source: 2026-07-27, after album-art Ken Burns fallback (TODO 26 step 4).
Base: Mixxx 2.5.6, branch `video`, 26 commits ahead of tag `2.5.6`.
Build: RelWithDebInfo, Ninja, vcpkg x64-windows-release.
Latest push: see `git log -1` on `origin/video`.

Verification standard: everything marked **Works** below has either been traced
from call site to effect, or is covered by a passing test, or both. Nothing is
marked Works on the strength of the code merely existing.

Status values:

- **Works**: traced or tested, no known defect.
- **Runs, wrong result**: executes and produces output, output is incorrect.
- **Dead**: compiles and is exposed in the UI or CO namespace, never executes.
- **Absent**: documented somewhere but does not exist in the tree.

## Test state

```
mixxx-test.exe                            876 tests (872 pass; 4 pre-existing ControllerScriptEngineLegacyTimer failures)
mixxx-test.exe --gtest_filter=VideoMixerTest.*     7 tests, all passing
mixxx-test.exe --gtest_filter=VideoFxChainTest.*   9 tests, all passing
mixxx-test.exe --gtest_filter=VideoFallbackTest.*  3 tests, all passing
mixxx-test.exe --gtest_filter=VideoPoolTest.*      5 tests, all passing
mixxx-test.exe --gtest_filter=VideoGenerativeTest.*  4 tests, all passing
mixxx-test.exe --gtest_filter=OscServerTest.*      2 tests, all passing
```

`src/test/videomixer_test.cpp` is new: 7 cases, registered under `if(FFMPEG)` in
CMakeLists.txt. Two are explicit regression tests naming the deck-key and
blend-curve defects.

The one previously failing test, `SoundSourceProxyTest.taglibStringToEnumFileType`,
was caused by this fork and is now fixed. See ASSESSMENT section 7.2.

## Video

| Feature | Status | Note |
|---|---|---|
| FFmpeg decode thread per deck | Works | real avcodec/avformat loop |
| Companion video autoload | Works | basename match, mp4 > mkv > mov > webm |
| Per-deck render, aspect letterbox | Works | QPainter, ~30fps repaint timer |
| A/V sync to engine position | Works | VideoSyncControl in EngineBuffer, 3-tier drift |
| `video_enabled` / `video_fullscreen` | Works | fullscreen still dupes COs, TODO 10 |
| `video_brightness` / `video_contrast` | Works | applied in paintEvent |
| `video_saturation` | **Works** (was Dead) | wired in paintEvent, covered by test |
| **Crossfader video mixing** | **Works** (was broken) | deck-keyed, blend curve corrected, tested |
| **Beat-locked video FX** | **Works** (MVP) | strobe + zoom pump via `VideoFxChain`; division 1/2/4/8/16/32; 9 tests |
| **Album-art Ken Burns fallback** | **Works** (step 4) | `VideoFallback` when no companion video; CO `video_fallback` (default on); 3 tests |
| **Beat-matched pool loops** | **Works** (step 2 MVP) | `VideoPool` scans settings `video-pool/` or `--video-pool`; BPM-scaled sync in `VideoDecoder`; 5 tests |
| **Generative beat visuals** | **Works** (step 3 MVP) | `VideoGenerative` CPU pulses/bars; CO `video_fallback_generative`; 4 tests |
| `[Master]` video output panel | Works | consumes the corrected blend |
| Non-yuv420p / NV12 sources | **Works** (was broken) | scaler built from actual frame format |
| Pause / resume | **Works** (was UB) | QWaitCondition now holds its mutex |
| Independent video crossfader | **Absent** | CO was dead and removed; TODO 23 |
| **NDI network output** | **Partial MVP** | `NdiOutput` stub without SDK; live send needs `-DNDI=ON` + SDK; not verified on receiver yet |
| Hardware decode (D3D11VA / CUDA) | **Dead** | no `get_format` callback, silently software |
| Video thumbnails in library | Absent | `VideoThumbnail` exists, not wired to CoverArt DAO |
| Stem separation | Compile OK (`ONNX_RUNTIME=ON`); not in default binary | Model + CO wiring still open — `docs/STEMS-ONNX-PROBE.md` |

Video output panel is hidden on a fresh **LateNight** profile (`show_video_output=0`).
**Mixxxxx Video** skin defaults both preview and output to on.

Video requires a legacy skin. `parseVideoWidget` returns nullptr under QML.
Mixxxxx Video skin: **Works** — `res/skins/MixxxxxVideo/` + Daylight scheme (`docs/SKINS.md`).

Beat FX COs (per `[ChannelN]`, registered when skin loads a `VideoWidget`):

| CO | Range | Effect |
|---|---|---|
| `video_beat_fx_strobe` | 0/1 | White flash on beat (first 15% of beat) |
| `video_beat_fx_zoom` | 0/1 | Zoom pump on beat |
| `video_beat_fx_division` | 0–5 | Maps to every 1/2/4/8/16/32 beats |
| `video_beat_fx_strobe_amount` | 0–1 | Strobe intensity (default 1) |
| `video_beat_fx_zoom_amount` | 0–1 | Zoom depth (default 1) |

Beat index is derived from `beat_closest` + `bpm` (seek-stable), not an accumulated counter.

Toggle via Developer Tools or `--set-control "[Channel1],video_beat_fx_strobe=1"`.
Still TODO: cut, RGB split, feedback trail (IDEAS.md full set).

## Phase indicator

| Feature | Status | Note |
|---|---|---|
| `[ChannelN],phase` CO | Works | |
| `WPhaseIndicator` arc widget | Works | in LateNight via row_4_overviewSpinny.xml |

## Export

| Feature | Status | Note |
|---|---|---|
| Trigger COs registered | Works | 7 push buttons, `coreservices.cpp:668` |
| Triggers connected to exporters | Works | `ExportController` ctor |
| Per-deck export | **Works** (was broken) | resolves real track path via PlayerManager |
| Crate export (GUI CO) | **Absent, fails loudly** | no crate selection via CO; use `--export-crate` CLI |
| Crate export (CLI) | **Works** | `--export-crate --export-format --export-path` |
| Crate import (CLI) | **Works** (playlist paths) | `--import-crate`; VDJ `.vdjfolder`, Serato `.crate`; not full Serato library |
| Hot cue export | **Works** (was broken) | uses `CueType::HotCue`, not literal 2 |
| Serato exporter class | Works as a library | not reachable from a trigger |
| VirtualDJ exporter class | Works as a library | writes database.xml, Camelot conversion |
| "Rekordbox" output format | **Mislabeled** | libdjinterop writes Engine, not Pioneer pdb |
| Export destination selection | Unsafe | heuristic can pick SD card, ISO, EFI partition |
| Per-track DB recreation in crate loop | Broken | `create_database` per track; TODO 15 |

## Control surface

| Feature | Status | Note |
|---|---|---|
| ControlObjects for all features | Works | inspectable via Developer Tools |
| MIDI / HID mapping | Works | upstream, untouched |
| Hercules DJ Console Mk1/Mk2 (~2003) | Works (upstream maps) | `Hercules DJ Console Mk1.hid.xml`, Mk2 MIDI/HID; shelf plug-in test pending (TODO 32) |
| **OSC server** | **Works** (MVP) | UDP 11119 in, 11118 out; `/mixxxxx/ping`; outbound includes fork `video_*` + `phase` |
| mixx-dj-mcp OSC bridge | **Works when Mixxx running** | probes `/mixxxxx/ping`, expects `/mixxxxx/pong` |

## Known build note

`mixxx.exe` will not relink while the app is running: `LNK1104: cannot open file
'mixxx.exe'`. Close Mixxx before building. `mixxx-lib.lib` and `mixxx-test.exe`
relink fine regardless, so tests can be run without closing the app.

## Documentation

| Doc | Status | Note |
|---|---|---|
| `docs/STATUS.md` | Current | this file — source of truth for Works/Absent |
| `docs/TODO.md` | Current | ordered backlog |
| `docs/NDI.md` | Current | NDI MVP (stub + SDK path); not verified on receiver |
| `docs/vj-integration-spout-osc.md` | Current | Spout + VJ OSC spec (TODO 28, after NDI) |
| `docs/SERATO-IMPORT.md` | Current | Serato `.crate` CLI/GUI paths (Dani workflow) |
| `docs/STEMS-ONNX-PROBE.md` | Current | TODO 31 ONNX compile probe |
| `docs/INSTALLER.md` | Current | branded NSIS/WIX pipeline (`just build-installer`) |
| `docs/SKINS.md` | Current | community install; MixxxxxVideo + Daylight |
| `docs/IDEAS.md` | Current | beat FX → fallback → NDI → VJ order |
| `docs/PROGRESS-20260726.md` | Current | session log |
| `docs/NOTES-20260726.md` | Current | decisions + glossary (NDI, skins, etc.) |
| `README.md` | Partial | doc table updated; OSC/hardware/export bodies still stale (TODO 21) |
| `PRD.md` | Stale | warning banner only (TODO 21) |
| mixx-dj-mcp Help → NDI tab | Current | mirrors `docs/NDI.md` summary |

## Doc accuracy

`README.md` and `PRD.md` still describe the OSC control table, hardware decode, and
Pioneer pdb output as working. They carry warning banners but have not been
rewritten. This file plus `docs/ASSESSMENT-20260726.md` are the source of truth
until TODO 21 is done.
