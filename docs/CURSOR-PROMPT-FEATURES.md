# Cursor Prompt: three feature expansions

Paste the block below into Cursor as the opening message. Everything in it has been
verified against the tree; the file and CO references are real.

Background reading it depends on: `docs/IDEAS.md`, `docs/STATUS.md`,
`docs/ASSESSMENT-20260726.md`.

---

## The prompt

```
You are working in D:\Dev\repos\mixxxxx, a fork of Mixxx 2.5.6 (branch `video`)
that adds video mixing to the DJ software. C++17, Qt6, CMake + Ninja, vcpkg.

Read these first, in order:
  docs/STATUS.md                 - what actually works vs what is dead or absent
  docs/IDEAS.md                  - full write-ups of the three features below
  docs/ASSESSMENT-20260726.md    - the defect history and why it happened
  docs/PROGRESS-20260726.md      - what changed most recently

=== GROUND RULES ===

This repo has one specific pathology: documentation written from intent, then
reused as evidence in the next review. Three features were marked "Working" in
the PRD while having ZERO call sites. A complete OSC control table was documented
for a server that does not exist anywhere in src/. Roughly 5000 lines shipped
with zero tests, and every single defect found later was a trivial one a first
test would have caught: an FFmpeg stream index used as a deck index, a group
string passed where a file path was expected, an enum compared against the wrong
integer.

Therefore:

1. Do not mark anything "working", "complete", or "done" that you have not run.
   If you cannot test it, say so plainly in your summary. A stub described
   honestly is fine. A stub described as finished is the failure mode this
   project is trying to break.
2. Every new feature lands with a test in src/test/. Use
   src/test/videomixer_test.cpp as the pattern: it is new, it passes, and two of
   its seven cases are explicit regression tests naming the bugs they catch.
3. Never register a ControlObject you do not read. Three dead COs were shipped
   this way (video_saturation, video_crossfader, and VideoDecoder::setSpeed).
   A live control in the UI that does nothing is worse than an absent one.
4. If you find something broken while working, fix it or write it down. Do not
   route around it silently.
5. Ask before making an architectural choice that is hard to reverse. Do not
   guess and proceed.

=== BUILD ===

Mixxx MUST be closed before building or the link fails with
"LNK1104: cannot open file 'mixxx.exe'".

  & "D:\Dev\repos\mixxxxx\build\cmake_build.cmd"

Note the leading `&` and full path; PowerShell needs it. From inside build\,
`.\cmake_build.cmd` also works. A bare `build\cmake_build.cmd` fails, because
PowerShell tries to autoload a module named `build`.

Tests:
  Set-Location "D:\Dev\repos\mixxxxx\build"
  .\mixxx-test.exe --gtest_brief=1                        # full suite, 858 tests
  .\mixxx-test.exe --gtest_filter=VideoMixerTest.*        # the video tests

The full suite currently passes 858/858. Keep it that way. If you break a test,
run `git diff 2.5.6 HEAD -- <the file>` before concluding the failure is
unrelated to your change; that exact assumption was wrong once already today.

Video sources live on the `mixxx-lib` target (CMakeLists.txt around line 3638),
which mixxx-test.exe also links, so new video code is unit-testable with no
build system changes. Test files register under `if(FFMPEG)` near line 2130.

=== WHAT ALREADY WORKS (verified, build on it, do not rewrite) ===

- VideoSyncControl (src/engine/controls/videosynccontrol.cpp) is a real
  EngineControl constructed inside EngineBuffer and addControl()-ed, so it runs
  on the per-buffer engine callback. It publishes true engine position in
  seconds to [ChannelN],video_audio_clock. This is the hard part of the fork and
  it is correct. Everything below leans on it.
- VideoDecoder: real FFmpeg decode thread per deck, companion-file autoload
  (basename + mp4/mkv/mov/webm), scaler built from the actual frame pixel format.
- VideoMixer: singleton, keyed on deck index (1-based, derived from the
  [ChannelN] group), composites two decks by crossfader position. Covered by
  videomixer_test.cpp.
- Beat controls, all confirmed present per deck: beat_distance (0.0-1.0),
  beat_closest, beat_next, beat_prev. src/engine/controls/phasecontrol.cpp
  already reads beat_distance and converts it to degrees; use that as your
  reference for reading these correctly.

=== WHAT IS BROKEN OR ABSENT (do not build on these) ===

- Hardware decode never engages. initHardwareDecoder() assigns hw_device_ctx but
  installs no get_format callback, so decode is silently software. Assume
  software decode when reasoning about performance.
- ONNX_RUNTIME=OFF in this build, so stem separation is NOT compiled in.
  src/stems/stem_separator.cpp has never been built in this configuration and
  its real state is unknown. Do not depend on stems.
- OSC server MVP exists (UDP 11118/11119, src/control/oscserver.cpp). README
  OSC table may still be stale — verify against code before wiring new clients.
- CLI: --set-control, --dump-controls, --gig-script, --export-crate,
  --import-crate (see docs/STATUS.md). Developer Tools CO browser still useful
  for live inspection (mixxx.exe --developer, legacy skin required).

=== THE WORK ===

Three features, described in full in docs/IDEAS.md. Implement them in this
order. Stop after each one, report honestly, and let me review before starting
the next. Do not attempt all three in one pass.

--- FEATURE 1: Beat-locked video FX (3-4 days) ---

Video effects that fire on the beat and on the phrase boundary, driven by the
beat grid the analyzer already computes for every library track.

Why this one first: the expensive infrastructure exists and is already correct.
VideoSyncControl gives sample-accurate deck position every engine buffer, and
beat_distance / beat_closest give beat phase. The hard part is done.

Build a VideoFxChain that reads beat_distance and beat_closest per deck and
drives parameterised effects. Design constraints:

  - Effects are small composable units, each with a beat-division parameter
    (every beat, every 4, every 16, every 32). Phrase-boundary awareness is what
    makes this musical rather than a strobe toy.
  - Apply effects in VideoMixer, not VideoWidget, so they survive into the
    [Master] output rather than only showing on the per-deck panel.
  - Starting effect set: cut, strobe, zoom pump, RGB split, feedback trail.
    Get two working properly before adding the rest.
  - Every effect parameter gets a ControlObject, and per rule 3 above, every one
    of those COs must actually be read.

Test: assert an effect fires at the expected beat_distance values and does not
fire between them. Effects are frame transforms, so you can test them on
synthetic QImages exactly the way videomixer_test.cpp does.

Reality check to keep in view: no other software does this automatically off an
analyzed library. Resolume charges for it and still needs manual per-track
mapping. That is the whole point of doing it here.

--- FEATURE 2: Video fallback chain (4-5 days) ---

Roughly 95% of any real DJ library has no companion video file. Today that means
a black rectangle, which means the feature goes unused. This is the actual
adoption blocker.

Per deck, walk a chain until something produces frames:
  1. Companion file (current behaviour, already works)
  2. Beat-matched loop from a tagged pool, selected by genre or energy
  3. Generative visuals
  4. Album art with slow Ken Burns motion

The part that makes it sing: time-stretch the pool loop to the deck BPM, so a
128 BPM visual loop plays correctly under a 174 BPM track. You have the deck
rate and the loop's own duration, so this is a playback rate multiplier, not DSP
work. Do not overbuild it.

Selection logic should use analyzer data that already exists (BPM, key, and
whatever energy or genre metadata the track carries). Do not add an analysis
pass.

Test: assert the chain falls through correctly when each tier is unavailable,
and that a loop's playback rate scales with deck BPM as expected.

--- FEATURE 3: NDI output (2-3 days) ---

Publish the mixed frame as an NDI source so Resolume, OBS, vMix, and club media
servers can pull video from mixxxxx over the network.

Today video output is a fullscreen window on a second monitor, which is a closed
box. This turns the project from "Mixxx with a video window" into a video source
that plugs into a rig that already exists. For a working VJ that is the
difference between a curiosity and a tool.

The NDI SDK is free and the API is close to create-sender / push-frame. The
frame already exists as a QImage in VideoMixer::blendFrame(), so this is largely
format conversion plus a send on a timer.

Make it CMake-optional (NDI=ON/OFF, default OFF) exactly like FFMPEG and
ONNX_RUNTIME are, so a build without the SDK still works. Ask me before adding
any new vcpkg dependency.

Test: assert the frame conversion produces correct dimensions and pixel layout.
Do not attempt to test actual network transmission in a unit test.

=== REPORTING ===

After each feature:
  - Update docs/STATUS.md using the existing status vocabulary: Works (traced or
    tested), Runs-wrong-result, Dead, Absent. No bare checkmarks, no new status
    words.
  - Append a session entry to docs/PROGRESS-<date>.md covering what you changed,
    what you verified and how, and anything you found broken along the way.
  - Move the corresponding item in docs/TODO.md (items 25, 26, 27) into the DONE
    table, with the same evidence-citing style used by the existing rows.

Start with Feature 1. Before you write any code, tell me your plan for
VideoFxChain: where it sits relative to VideoMixer, how effects are composed,
and how the beat-division parameter is expressed. I want to review that before
implementation.
```

---

## Notes on why the prompt is shaped this way

**It front-loads the failure history.** An agent that does not know this repo shipped
three dead ControlObjects and a fictional OSC table will cheerfully add a fourth. The
ground rules are specific and cite real incidents rather than offering generic advice
about code quality.

**It names what is already correct.** Without that, an agent is liable to rewrite
`VideoSyncControl`, which is the most valuable and hardest-won code in the fork.

**It names what is broken.** Hardware decode not engaging and `ONNX_RUNTIME=OFF`
matter here: an agent reasoning about frame-rate budget will otherwise assume GPU
decode, and one reaching for stems will find a file that has never been compiled.

**It reflects 2026-07-26 session state.** OSC MVP and CLI export/import exist;
re-read `docs/STATUS.md` before assuming those are absent.

**It forces a plan before code on Feature 1.** Where `VideoFxChain` sits relative to
`VideoMixer` determines whether effects reach the master output. That is expensive to
reverse later.

**It stops after each feature.** Three features in one pass produces a large
unreviewable diff, which is exactly how this codebase accumulated 5000 untested
lines in the first place.
