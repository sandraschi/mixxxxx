# Mixxxxx Status

Last verified against source: 2026-07-26, after the first fix pass.
Base: Mixxx 2.5.6, branch `video`, 18 commits ahead, worktree modified.
Build: RelWithDebInfo, Ninja, vcpkg x64-windows-release.

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
mixxx-test.exe                            858 tests, all passing
mixxx-test.exe --gtest_filter=VideoMixerTest.*     7 tests, all passing
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
| `[Master]` video output panel | Works | consumes the corrected blend |
| Non-yuv420p / NV12 sources | **Works** (was broken) | scaler built from actual frame format |
| Pause / resume | **Works** (was UB) | QWaitCondition now holds its mutex |
| Independent video crossfader | **Absent** | CO was dead and removed; TODO 23 |
| Hardware decode (D3D11VA / CUDA) | **Dead** | no `get_format` callback, silently software |
| Video thumbnails in library | Absent | `VideoThumbnail` exists, not wired to CoverArt DAO |
| Stem separation | Absent from binary | `ONNX_RUNTIME=OFF` in this build |

Video output panel is hidden on a fresh profile: `[Skin],show_video_output` is
`persist="true"` and defaults to 0 (`skin.xml:44`).

Video requires a legacy skin. `parseVideoWidget` returns nullptr under QML.

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
| Crate export | **Absent, fails loudly** | no crate selection exists; TODO 12 |
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
| **OSC server** | **Absent** | zero OSC code in `src/`; TODO 18 |
| mixx-dj-mcp OSC bridge | Sends into a void | UDP never errors, tools report false success |

## Known build note

`mixxx.exe` will not relink while the app is running: `LNK1104: cannot open file
'mixxx.exe'`. Close Mixxx before building. `mixxx-lib.lib` and `mixxx-test.exe`
relink fine regardless, so tests can be run without closing the app.

## Doc accuracy

`README.md` and `PRD.md` still describe the OSC control table, hardware decode, and
Pioneer pdb output as working. They carry warning banners but have not been
rewritten. This file plus `docs/ASSESSMENT-20260726.md` are the source of truth
until TODO 21 is done.
