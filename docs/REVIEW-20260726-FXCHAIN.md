# Code Review: VideoFxChain (in progress) + session verification

**Written:** 2026-07-26, late session
**Reviewer:** read-only pass. No builds, no edits to source. Findings only.
**Scope:** the uncommitted `VideoFxChain` work, plus verification of the five
commits from `a3833e6fbb` to `7631d04b11`.

---

## 0. Note on a concurrent build

A background `ninja mixxx-test` was launched in `build/` from a review session at
roughly 23:25. It targeted only `mixxx-test` and its first attempt failed with
`LNK1104: cannot open file 'mixxx-test.exe'`. If an unexplained link failure shows
up around that timestamp, that is the cause, not your change. Nothing was edited.

Separately: a `mixxx` process (PID 91184) was running. That blocks any `mixxx.exe`
relink with `LNK1104`. Close the app before a full build.

---

## 1. Verification of committed work

Checked against source, not against the docs. All of this holds up.

| Claim | Verified how | Result |
|---|---|---|
| OSC server exists | `src/control/oscserver.{h,cpp}` present, `QUdpSocket` both directions | **Real** |
| OSC is wired, not orphaned | constructed + `start()` + `stop()` in `coreservices.cpp`, member in `coreservices.h` | **Real** |
| OSC registered in build | `src/control/oscserver.cpp` and `src/test/oscserver_test.cpp` in CMakeLists | **Real** |
| OSC tested | 2 tests pass (9 total with VideoMixerTest) | **Real** |
| CLI flags | `set-control`, `dump-controls`, `gig-script`, `export-crate`, `import-crate`, `export-format`, `export-path` all in `cmdlineargs.cpp` | **Real** |
| FX COs registered | `video_beat_fx_strobe`, `_zoom`, `_division` created, not just read | **Real** |
| FX applied to master output | `VideoFxChain::applyForDeck` called from `VideoMixer::blendFrame` on both the single-deck and two-deck paths | **Correct placement** |

Good work. The OSC server was the largest outstanding gap in the project and it is
genuinely implemented rather than stubbed.

---

## 2. Bugs in VideoFxChain, by severity

### 2.1 CRITICAL: pixel loops run inside the mixer mutex

`VideoMixer::blendFrame()` opens with `QMutexLocker lock(&m_mutex)`. Inside that
lock it now calls `VideoFxChain::applyForDeck()` for deckA and deckB.
`applyStrobe()` is a per-pixel scanline loop over the entire frame.

So the GUI thread performs roughly 2M pixel operations per deck while holding the
lock that `VideoDecoder::pushFrame()` needs to deliver frames. Both decoder threads
stall behind GUI-thread image processing, every frame.

**Fix:** copy the source frames out under the lock, release it, then apply FX.
`blendFrame` should hold the mutex only for map access, never for pixel work.

### 2.2 HIGH: beat counter never resets, so phrase alignment silently drifts

`DeckState::beatCounter` only ever increments, from an arbitrary origin: whenever
rendering first started. It is never reset on seek, hotcue jump, or track load.

Since `beatFxActive()` is `beatCounter % division == 0`, "every 16 beats" means 16
beats counted from an arbitrary point, **not** from the downbeat. One hotcue jump
and the phrase FX are permanently out of phase with the music.

Phrase alignment is the entire point of this feature. This defect makes it look like
it works in a short test and fall apart in a real set, which is the worst failure
shape.

**Fix:** derive an integer beat index from track position against the beat grid,
rather than accumulating edges. `beat_closest` plus the beat grid gives an absolute
beat number that survives seeks. Reset or recompute on `track_loaded` and on seek.

### 2.3 MEDIUM: shared mutable state across two independent render paths

`deckStates()` is a global static `QMap`, and `applyForDeck()` is called from
`VideoWidget::paintEvent()` **and** twice from `VideoMixer::blendFrame()`.

`advanceBeatCounter()` is edge-triggered on `lastBeatDistance` crossing 0.5, so
whichever caller runs first consumes the edge and the others see no advance.
Consequence: showing or hiding the deck preview panel changes the beat counting
behaviour of the master panel. That produces irreproducible "it worked yesterday"
bugs.

**Fix:** falls out of 2.2. If the beat index is derived from track position rather
than accumulated, the function becomes stateless and call order stops mattering.

### 2.4 MEDIUM: strobe window is narrower than the frame interval

`kWindow = 0.15` of a beat. At 174 BPM that is about 52ms. The repaint timer runs at
roughly 30fps, so 33ms per frame. The window is about 1.5 frames wide, meaning
whether a strobe fires at all depends on where the repaint happens to land.

Expect visibly inconsistent strobing, worse at higher BPM. At 200 BPM the window is
roughly one frame and misses become common.

**Fix:** make the window duration-based (a fixed number of milliseconds, or at
minimum clamped to at least 2 frame intervals) rather than a fixed fraction of a
beat.

### 2.5 LOW: allocation churn

`applyStrobe()` calls `convertToFormat()` (full copy) and `applyZoomPump()`
allocates a fresh `QImage`, per deck per frame. At 1080p/30fps across two decks that
is roughly 250 MB/s of allocation and free.

**Fix:** strobe can operate in place when the source is already RGBA8888, which it
is coming out of `VideoDecoder`. Zoom needs a destination buffer, but it can be
cached per deck and reused.

### 2.6 LOW: no intensity control

There are COs for strobe on/off, zoom on/off, and division, but nothing to set the
amount. `boost = strength * 200.0` and `1.0 + 0.12 * ...` are hardcoded.

The first thing anyone will ask for is a knob. Add `video_beat_fx_strobe_amount` and
`video_beat_fx_zoom_amount`, both 0..1, and read them. Note the existing project
rule: do not register a CO you do not read.

---

## 3. Documentation drift

**`STATUS.md` still reports "858 tests, all passing".** That was the count before
`oscserver_test.cpp` existed. It should be 860 now, and higher once
`videofxchain_test.cpp` is committed.

Small, but it is exactly the drift pattern the status discipline exists to prevent:
a number carried forward instead of recomputed. Please re-run the suite and take the
real count rather than incrementing by hand.

**Unverified claim worth spot-checking.** `CHANGELOG.md` says `--export-crate`
supports "engine, Serato, VirtualDJ formats". Earlier analysis established that only
libdjinterop (Engine Library) actually writes; `SeratoExporter` and
`VirtualDjExporter` exist as library classes that were not reachable from any
trigger. Confirm `--export-format serato` produces a valid `.crate` on disk rather
than silently doing nothing, and if it does not, downgrade the claim.

---

## 4. Suggested order from here

1. Fix 2.1 (mutex) and 2.2 (beat origin). These are the two that matter. 2.3 falls
   out of 2.2 for free.
2. Fix 2.4 (strobe window), since it directly affects whether the feature looks good.
3. Commit Feature 1 with its test, update STATUS with a recomputed test count.
4. Then consider the shader path below **before** starting Feature 2.

---

## 5. Ideas worth considering, roughly by leverage

### 5.1 GPU shader FX via Qt6 QRhi

**Recommended as the next architectural move after Feature 1 lands.**

This retires bugs 2.1 and 2.5 structurally instead of patching them, and unlocks
effects that are not viable on CPU at all: feedback trails, displacement maps,
chroma key, real-time kaleidoscope, per-pixel colour grading.

A GLSL fragment shader taking beat phase as a uniform is both dramatically faster
and far more expressive than a scanline loop. Doing this before adding more CPU
effects avoids building a pile of code that has to be thrown away.

### 5.2 Mixxxxx as the master beat clock for the rig

Now that OSC exists, this is close. Publish beat and phrase position outbound as
OSC, and Resolume, TouchDesigner, and VDMX can all follow the mixxxxx beat grid.

That reframes the project: not a video player, but the **clock source** for a whole
visual rig. Add Ableton Link and it syncs to anything else in the room.

Small amount of code on top of infrastructure that already exists. High strategic
payoff.

### 5.3 Art-Net / DMX lighting output

Identical engine to the beat FX, different sink. Beat-locked lighting is a
several-thousand-euro feature in commercial software, and the sample-accurate beat
position is already there.

Makes the FX architecture pay for itself twice, and small venues would genuinely
install it.

### 5.4 The auto-clip gig recorder

Record the master video mix, then use the beat grid and energy analysis to
automatically cut 30 second clips around drops and export them vertical for socials.

Every DJ wants content from their set. Nobody wants to edit it. This is the feature
most likely to get the project talked about.

### 5.5 Video DVS

Scratch the video with the record. Frame-accurate sync to engine position already
exists, so the video should follow the platter through scratches.

Nobody has shipped this properly. Genuinely novel, and an unbeatable demo.

Caveat from the earlier assessment: the current sync path uses a 2 second hard-seek
threshold and `av_seek_frame` lands only on keyframes, so real scratch response
needs a different seek strategy. Worth prototyping before promising.

---

## 6. Standing rules, restated

Unchanged from the original brief, repeated because they are what this review was
measuring against:

- Do not mark anything working that you have not run.
- Every feature lands with a test.
- Never register a ControlObject you do not read.
- If you find something broken, fix it or write it down. Do not route around it.
---

## 7. Fixes applied (same session, post-review)

| Finding | Fix |
|---|---|
| **2.1** mutex held during pixel loops | `blendFrame()` copies frames under lock, releases, then applies FX |
| **2.2** beat counter drift on seek | `beatIndexFromGroup()` from `beat_closest` + `bpm` — stateless |
| **2.3** dual render paths fighting | Falls out of 2.2 — no shared edge accumulator |
| **2.4** strobe window too narrow | Minimum 66ms window (~2 frames @ 30fps), scales with BPM |
| **2.6** no intensity COs | `video_beat_fx_strobe_amount`, `video_beat_fx_zoom_amount` (0–1, read) |

**2.5** (allocation churn) and **5.x** (shader path, OSC beat clock) remain backlog — see TODO 25.

Re-run `mixxx-test.exe` after build for authoritative test count in STATUS.md.

