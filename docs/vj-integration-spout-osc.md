# VJ Integration: Spout Video Sharing + OSC Control Surface

**Status**: spec, not implemented
**Date**: 2026-07-27
**Target**: mixxxx fork, D:\Dev\repos\mixxxxx
**Audience**: Cursor / coding agent. Read the whole file before touching code.

---

## 1. Goal

Make mixxxx interoperable with external VJ software without buying Resolume (Avenue EUR 299 /
Arena EUR 799, plus EUR 400 for Wire if you want ISF shaders). Two independent features:

1. **Spout**: share composited video frames with other Windows apps, GPU-to-GPU, zero copy to disk.
2. **OSC**: publish deck state (BPM, beat phase, play, crossfader, track metadata) over UDP so any
   VJ tool can react to the set.

With both in place, a EUR 40 tool (Magic Music Visuals) or a free one (TouchDesigner
Non-Commercial, Hydra) becomes a usable partner, and mixxxx is the clock master rather than a
passive audio source.

**Non-goal**: replacing Resolume's clip grid. Not building a VJ instrument here, just the plumbing.

---

## 2. Current state of the fork (verified 2026-07-27)

Read these before planning:

- `src/video/videodecoder.{h,cpp}` - decode to frames
- `src/video/videomixer.{h,cpp}` - singleton, composites per-deck frames by crossfader position
- `src/video/videofxchain.{h,cpp}` - beat-locked strobe/zoom FX
- `src/video/videowidget.{h,cpp}` - per-deck display widget
- `src/video/videopool.{h,cpp}`, `videofallback.{h,cpp}` - fallback/loop sources

**Critical architectural fact**: the pipeline is CPU-side `QImage`, not GPU textures.

- `VideoWidget : public QWidget` renders via `paintEvent()` + `QPainter`, not `QOpenGLWidget`.
- `VideoMixer::pushFrame(int deck, const QImage&, double pts)` and
  `QImage VideoMixer::blendFrame(double crossfader)` are the compositing core.
- `VideoFxChain::applyForDeck(int deck, const QImage& src)` returns `QImage`.

Consequence: there is no OpenGL texture to hand to Spout. Do NOT plan around `spoutGL` /
`SendTexture()` / NV_DX_interop. See section 4.

---

## 3. What Spout actually is

Spout (https://github.com/leadedge/Spout2) is the Windows equivalent of Syphon on macOS: a
frame-sharing system that lets one application publish video frames and any number of other
applications subscribe, in real time, on the same machine.

- **License**: BSD-2-Clause. Compatible with Mixxx's GPL. Safe to link and ship.
- **Latest release**: 2.007.017, 22 October 2025. Repo is alive, ~980 stars, PRs go to the `beta`
  branch.
- **Supports**: OpenGL, DirectX 9, DirectX 11, DirectX 12.
- **Windows only.** No Linux or macOS equivalent in this API. Upstream Mixxx would reject this as
  a platform-specific feature; we are a fork, so we guard it behind `if(WIN32)` and move on.

### How it works under the hood

The universal exchange format is a **DirectX 11 shared texture**. A sender creates a shared DX11
texture and registers its name in a shared-memory sender registry. Receivers look up the name,
open the shared texture handle, and read it directly on the GPU. No frame ever crosses the PCIe
bus back to system RAM if both sides are GPU-side.

OpenGL applications reach that DX11 texture through the `WGL_NV_DX_interop2` extension. That is
why Spout has a GL/DX interop layer at all, and why the classic failure mode is the dialog
"Cannot create DirectX/OpenGL interop": sender and receiver ended up on different graphics
adapters. On a single-GPU desktop (RTX 4090, Goliath) this is not an issue. On laptops with
hybrid graphics you must force both apps to the same adapter via Windows graphics settings.

Spout falls back to CPU staging textures when GPU interop is unavailable, so it degrades rather
than fails.

### The library layout

| Library | Use case | Header |
|---|---|---|
| `SpoutGL` | OpenGL apps, uses GL/DX interop | `Spout.h`, `SpoutSender.h`, `SpoutReceiver.h` |
| `SpoutDX` | DirectX 11 apps, no OpenGL dependency | `SpoutDX.h` |
| `SpoutDX9` / `SpoutDX12` | Legacy / newer DX | - |
| `SpoutLibrary` | C-compatible DLL for non-MSVC compilers | `SpoutLibrary.h` |

Docs (authoritative, read before writing calls):
- OpenGL SDK: https://spoutgl-site.netlify.app/
- DirectX classes: https://spoutdx-site.netlify.app/
- C library: https://spoutlibrary-site.netlify.app/

---

## 4. Which API to use here, and why

**Use `SpoutDX`, not `SpoutGL`.**

Reasoning: our frames are `QImage` in system memory. `spoutGL` requires an active OpenGL context
on the calling thread for everything it does, including its pixel-upload paths. We do not have one
and creating one just to feed Spout is a waste. `spoutDX` creates its own DX11 device internally
and accepts a raw pixel buffer, no OpenGL anywhere.

The relevant `spoutDX` calls are approximately:

```cpp
// VERIFY exact signatures against https://spoutdx-site.netlify.app/ before writing code.
// The API has stable names but parameter defaults have moved across 2.007.x releases.
spoutDX sender;
sender.SetSenderName("mixxxx Master");
sender.SendImage(pixels, width, height);   // BGRA / RGBA buffer, see section 6
sender.ReleaseSender();
```

If we later move the video pipeline to GPU textures (QRhi or QOpenGLWidget), revisit and switch to
`SendTexture()`. Note that as of 2.007.017 there is an experimental branch exploring a new
OpenGL/DirectX interop method, so the GL path is still moving. Another reason to stay on `spoutDX`.

---

## 5. Build integration

Mixxx already builds with CMake + vcpkg on Windows, so this is cheap.

1. Add `spout2` to the vcpkg manifest (check `vcpkg.json` / the buildenv setup in
   `D:\Dev\repos\mixxxxx\buildenv`).
2. In CMake:

```cmake
if(WIN32)
  find_package(Spout2 CONFIG REQUIRED)
  target_link_libraries(mixxx-lib PRIVATE Spout2::SpoutDX_static)
  target_compile_definitions(mixxx-lib PRIVATE __MIXXXX_SPOUT__)
endif()
```

   Available targets from the vcpkg port: `Spout2::Spout`, `Spout2::Spout_static`,
   `Spout2::SpoutLibrary`, `Spout2::SpoutDX`, `Spout2::SpoutDX_static`.
   Port is `windows-x64` and `windows-x86` only, no linux/osx/uwp/arm64.

3. **Check the vcpkg port version.** It has lagged upstream before (port was on 2.007.010 while
   upstream shipped 2.007.017). If the port is stale, vendor the SDK sources under `lib/spout2/`
   instead. Only 4 source folders are needed for SpoutDX, and the repo ships its own CMakeLists.
4. Every Spout call site goes behind `#ifdef __MIXXXX_SPOUT__`. The Linux build must stay green.
5. Add the Spout BSD-2-Clause notice to the distribution licence file. Do not skip this, BSD-2
   requires reproducing the copyright notice in binary distributions.

---

## 6. Implementation plan: Spout sender

### New files

```
src/video/spoutsender.h
src/video/spoutsender.cpp
```

### Class sketch

```cpp
// src/video/spoutsender.h
#pragma once
#ifdef __MIXXXX_SPOUT__

#include <QImage>
#include <QMutex>
#include <QThread>
#include <QWaitCondition>
#include <QString>

// Publishes composited frames to a named Spout sender.
// One instance per sender name. Runs its own thread; spoutDX is NOT thread safe,
// so the spoutDX object is created, used and destroyed on that thread only.
class MixxxxSpoutSender : public QThread {
    Q_OBJECT
  public:
    explicit MixxxxSpoutSender(const QString& senderName, QObject* parent = nullptr);
    ~MixxxxSpoutSender() override;

    // Called from the GUI thread. Non-blocking: copies into the pending slot
    // and wakes the sender thread. Drops the previous frame if it has not been
    // sent yet (latest-wins, we are a live video feed, not a queue).
    void submitFrame(const QImage& frame);

    void stop();

  protected:
    void run() override;   // owns the spoutDX instance

  private:
    QString m_senderName;
    QMutex m_mutex;
    QWaitCondition m_wake;
    QImage m_pending;      // always Format_RGBA8888 or Format_ARGB32, see below
    bool m_hasPending = false;
    bool m_stopping = false;
};

#endif // __MIXXXX_SPOUT__
```

### Hook point

`VideoMixer` is the correct tap. It is the singleton that already produces the master composite and
the comment in `videofxchain.h` confirms FX are applied there so they "survive into the master
output panel". That is exactly the frame a VJ tool should receive.

In `VideoMixer::blendFrame()`, after FX are applied, before returning:

```cpp
#ifdef __MIXXXX_SPOUT__
    if (m_pSpoutMaster) {
        m_pSpoutMaster->submitFrame(out);
    }
#endif
```

Add lazy creation of `m_pSpoutMaster` gated on a ControlObject (see section 9), so the sender only
exists when the user turns it on. Creating a DX11 device costs real time and VRAM, do not do it
unconditionally at startup.

**Optional phase 2**: per-deck senders ("mixxxx Deck 1", "mixxxx Deck 2") tapped in
`VideoMixer::pushFrame()`. This lets an external tool do its own blending with mixxxx's crossfader
position delivered over OSC. Costs one DX11 shared texture per deck. Do not build this until the
master sender works.

### Pixel format and orientation

This is where it will break first, so be deliberate:

- `QImage::Format_ARGB32` on little-endian x86 is **BGRA in byte order**.
- `QImage::Format_RGBA8888` is **RGBA in byte order** regardless of endianness.
- DirectX shared textures are typically `DXGI_FORMAT_B8G8R8A8_UNORM`, so `Format_ARGB32` is
  usually the cheaper match. Confirm what `SendImage` expects in the version you link against.
- Convert once with `QImage::convertToFormat()` and only if the format differs. Do not convert
  every frame if `blendFrame` already produces the right format. Check what `VideoMixer` outputs.
- `QImage` scanlines run top-down. DirectX textures are also top-down, OpenGL is bottom-up.
  `SendImage` has an invert flag. If receivers show the image upside down, that flag is the fix,
  not a manual flip in our code.
- Call `frame.constBits()` and respect `frame.bytesPerLine()`. Qt pads scanlines to 4-byte
  boundaries; for 32bpp formats stride is always width*4, but assert it rather than assume.

### Performance budget

CPU-side sending means one memcpy into a DX11 staging texture per frame.

- 1280x720 RGBA = 3.7 MB/frame. At 60 fps that is 221 MB/s.
- 1920x1080 RGBA = 8.3 MB/frame. At 60 fps that is 498 MB/s.

Both are fine on Goliath, but they are not free and they must not happen on the GUI thread during
`paintEvent`. That is the entire reason `MixxxxSpoutSender` owns a thread. Measure with a frame
counter before and after; if GUI frame time regresses, the submit path is copying when it should be
handing off.

Latest-wins semantics: if the sender thread is behind, overwrite the pending frame rather than
queueing. A VJ feed that is three frames stale is worse than one that dropped two frames.

### Frame sync

Spout has `SetFrameSync()` / `WaitFrameSync()` and an `HoldFps()` helper for pacing. Ignore them in
phase 1. Add only if a receiver reports tearing.

---

## 7. Implementation plan: Spout receiver (phase 3, optional)

The inverse direction is more interesting than it sounds: it lets a shader tool generate visuals
and mixxxx composite them as a video layer, so mixxxx stays the video mixer and the crossfader
still means something.

- Use `spoutDX::ReceiveImage()` into a `QImage`, feed it into `VideoMixer` as a synthetic deck or
  as a new layer type alongside `VideoPool`.
- Sender discovery: Spout maintains a sender registry; enumerate it to populate a combo box in
  preferences rather than making the user type a name.
- Do NOT build this before the sender ships and is proven at a gig.

---

## 8. OSC control surface

Independent of Spout, and honestly the higher-value half. Every VJ tool in the target list speaks
OSC (Magic, Synesthesia, TouchDesigner, VVVV, ossia).

### What already exists (2026-07-26 MVP — do not rebuild)

`src/control/oscserver.{h,cpp}` is **live**:

- **Inbound** UDP **11119** — deck/crossfader/effect ControlObject mapping (mixx-dj-mcp control).
- **Outbound** UDP **11118** — mirrors a small subscribed CO set + `/mixxxxx/pong` heartbeat.
- Config: `[Osc],enabled`, `port_in`, `port_out`, `host_out`.

This is a **remote control bridge**, not a VJ clock surface. Phase A of TODO 28 **extends**
this server (or factors shared encode/decode into `src/network/oscmessage.{h,cpp}`) with the
schema below. Do **not** start a second `QUdpSocket` or duplicate ports.

### Existing fallback (zero code)

Upstream Mixxx already ships a "MIDI for light" controller script that emits MIDI clock output. It
loads like a normal controller mapping, auto-detects the current deck, and needs no intervention
during a set. Point it at a loopback MIDI port and any MIDI-clock-following tool syncs to the set
today. Document this in the README as the no-build option. On Windows 11 the new Windows MIDI
Services stack gives multi-client ports natively, so no loopMIDI install is needed.

OSC is strictly better because it carries semantics, not just tempo.

### Message schema (proposal, bikeshed before implementing)

```
/mixxxx/deck/<n>/play         i   0|1
/mixxxx/deck/<n>/bpm          f
/mixxxx/deck/<n>/beat_distance f  0.0-1.0, phase within the beat
/mixxxx/deck/<n>/position     f   0.0-1.0
/mixxxx/deck/<n>/pitch        f
/mixxxx/deck/<n>/volume       f
/mixxxx/deck/<n>/track        s   "Artist - Title"
/mixxxx/deck/<n>/key          s
/mixxxx/master/crossfader     f   -1.0-1.0
/mixxxx/master/bpm            f   bpm of the deck currently dominant
/mixxxx/master/vu             f   0.0-1.0
/mixxxx/beat                  i   deck number, fired on each beat of the dominant deck
```

### Wiring

- Source of truth is the existing ControlObject system. Use `ControlProxy` on `[ChannelN],bpm`,
  `beat_distance`, `beat_closest`, `play`, `playposition`, `rate`, `volume`, and
  `[Master],crossfader`. `VideoFxChain` already reads these, copy its pattern.
- Track metadata via `PlayerManager` / deck `TrackPointer`, event-driven on track load, not polled.
- Continuous values at a fixed 50 Hz timer. Discrete values event-driven. Do not send on every
  ControlObject change, `beat_distance` changes every audio buffer and would flood the socket.
- Transport: plain UDP via `QUdpSocket`. No extra dependency. Reuse the existing
  `OscServer` send socket; add a dedicated VJ target host/port under `[VJ]` if the
  mixx-dj-mcp bridge port (11118) should stay separate.

### OSC encoding

Do NOT pull in liblo (LGPL, awkward on Windows) or oscpack. The OSC 1.0 wire format is trivial:

- Address pattern: null-terminated string padded to a multiple of 4 bytes.
- Type tag string: `,` followed by one char per argument (`i` int32, `f` float32, `s` string),
  same null-pad-to-4 rule.
- Arguments: int32 and float32 big-endian, strings null-terminated and padded to 4.

A correct encoder is about 120 lines including tests. Put it in `src/network/oscmessage.{h,cpp}`
with unit tests in `src/test/`. Hand-rolling is the right call here: one fewer vcpkg port, one
fewer licence to audit, and the format has not changed since 2002.

### Config

Preferences page: enable checkbox, target host (default 127.0.0.1), target port (default 9000),
update rate. Store under the existing `ConfigObject` keys, group `[VJ]`.

---

## 9. ControlObjects to add

Follow the naming already used for video COs in `videowidget.h`:

```
[VJ],spout_enabled        toggle, creates/destroys the master Spout sender
[VJ],spout_deck_senders   toggle, phase 2
[VJ],osc_enabled          toggle
```

Exposing these as COs (not just preferences) means they are MIDI-mappable and scriptable, which
matters for a live rig where you want to kill the video feed from a controller button.

---

## 10. Testing

1. **Spout smoke test**: download the Spout distribution (SPOUT-2007 zip from the releases page),
   run the bundled DEMO receiver. It exists precisely to establish whether a system supports
   texture sharing. Confirm it sees "mixxxx Master" and shows the composite.
2. **Real receiver**: OBS with the obs-spout2 plugin, or Magic Music Visuals trial. If OBS shows
   the feed, every downstream tool will.
3. **Format check**: play a video with a strong colour cast (heavy red) and confirm it is not blue
   in the receiver. Blue means RGBA/BGRA are swapped.
4. **Orientation check**: any clip with text. Upside down means the invert flag.
5. **Performance**: enable/disable `[VJ],spout_enabled` mid-playback and compare GUI frame times.
   Regression on the GUI thread is a bug in the submit path, not a Spout limitation.
6. **OSC**: verify with any OSC monitor before touching a VJ app. Protokol or a 20-line Python
   `python-osc` listener. Confirm `beat_distance` looks like a sawtooth at the track BPM.
7. **End to end**: mixxxx sends Spout + OSC, receiver reacts to `/mixxxx/deck/1/beat_distance`.
   That is the ship criterion.

---

## 11. Known pitfalls

- **Adapter mismatch**: sender and receiver on different GPUs silently fails or throws the interop
  error dialog. Single-GPU box, non-issue, but document it for laptop users.
- **spoutDX is not thread safe.** One instance, one thread. Do not share it across the master and
  per-deck senders; give each its own.
- **Antivirus false positives**: the Spout distribution zip has historically tripped Defender on
  `SpoutPanel.exe`. That is the utility zip, not the SDK we link, but expect it when downloading
  the demo tools.
- **Registry settings**: Spout stores config under `HKEY_CURRENT_USER\Software\Leading Edge\Spout`.
  The utilities write these. Our app should not, and should not require them to exist.
- **Do not stub and claim done.** A sender that registers a name but publishes black frames looks
  like success in the registry and fails at the gig. Test 1 is mandatory.

---

## 12. Verify before writing code

The following were correct on 2026-07-27 and are the things most likely to have moved:

- [ ] Exact `spoutDX::SendImage()` / `ReceiveImage()` signatures at https://spoutdx-site.netlify.app/
- [ ] vcpkg `spout2` port version vs upstream release (was 2.007.017, Oct 2025)
- [ ] Whether `VideoMixer::blendFrame()` output format is already ARGB32
- [ ] Whether the fork's `vcpkg.json` manifest is the actual dependency source on this machine
- [ ] Qt version in use, since QImage format enums are stable but `constBits()` constness is not
      across Qt5/Qt6

---

## 13. Sequencing

**Fleet order (agreed 2026-07-26, reaffirmed 2026-07-27):** finish fallback chain → **NDI (TODO 27)**
→ **this spec (TODO 28)**. NDI serves OBS/Kick (network); Spout serves same-PC VJ tools.

Within TODO 28:

1. OSC-out for VJ (phase A). Extend existing `OscServer`. No new dependency, no GPU risk.
   Half a day.
2. Spout master sender (phase B). One to two days including format/orientation debugging.
3. Preferences UI and `[VJ]` COs (phase C). Half a day.
4. Per-deck senders and receiver path (phase D) — only after gig-testing 1–3.

Do not start phase B before phase A works. Do not start phase D until you have VJ'd a set with A and B.
