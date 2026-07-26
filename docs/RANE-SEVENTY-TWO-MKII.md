# Rane Seventy-Two MKII: Support Assessment

Written 2026-07-26. Research and feasibility, no implementation yet.
Task brief for implementation is in `docs/HANDOFF-RANE-MAPPING.md`.

## Current support: none

- No Rane mapping of any kind in `res/controllers` (142 mappings present; the only
  name match was `KANE_QuNeo`, a false positive on the substring).
- Nothing upstream in Mixxx either. The nearest community effort is a work in
  progress journal for the Rane **Twelve** MK2 on the Mixxx forum, which is the
  turntable, not the mixer.
- A 2022 forum thread asks specifically about the Seventy-Two with waveform display
  and went unanswered.

## "Connecting the 72" is three separate problems

Splitting them matters, because they differ by roughly two orders of magnitude in
difficulty and only two of them are worth doing.

### A. Audio interface: likely straightforward

The Seventy-Two MKII is a USB audio interface with dual DVS inputs and RCA timecode
outputs.

- macOS is class compliant and needs no manufacturer driver.
- **Windows requires Rane's driver** from their site. Not plug and play.
- This build's vcpkg PortAudio has WASAPI and WDM-KS confirmed present. An `ASIO`
  string appears in `portaudio.dll` but that is weak evidence and has not been
  confirmed as a working host API. **Verify in Preferences, Sound Hardware, Sound
  API dropdown.** If ASIO is listed, prefer it. Otherwise WASAPI exclusive mode.
- `VINYLCONTROL=ON` in this build, so DVS is available.

### B. MIDI control surface: buildable

The key de-risking fact: **VirtualDJ has a full mapping of the Seventy-Two MKII.**
Rane deliberately opened the MKII generation beyond Serato, and the hardware sends
control data over USB MIDI plus timecode over the RCA outputs to Serato DJ Pro,
VirtualDJ, and Traktor Pro. So the mixer emits MIDI that a non-Serato host can read.
That was the single largest unknown and it is answered.

**Known limitation, from DJs working on the VDJ mapping:** the Seventy-Two's
firmware was tailored to the way effects work in Serato, and unlike the Rane Four it
does **not** send FX ASSIGN data to software. Expect the FX section to be partially
or entirely unmappable. Faders, EQ, trim, crossfader, and the 16 performance pads
should be fine.

Do not promise a complete mapping. Promise a mapping with a documented FX gap.

### C. Touchscreen and waveform display: do not attempt

VirtualDJ does drive waveforms onto the built-in display. That is a proprietary USB
protocol, almost certainly implemented with Rane's cooperation. Reverse engineering
it would be weeks of USB captures with no guarantee of success and a real chance
that a firmware update breaks it.

Treat the screen as a Serato and VDJ feature. Mixxx's own waveforms run on the
laptop screen. This is not a gap worth closing.

## Recommended order of work

**DVS first, MIDI second.** For a battle mixer plus turntables, timecode is the
high-value path and needs no mapping at all:

1. Install the Rane Windows driver.
2. Select the mixer as the sound device in Mixxx, confirm the API in the dropdown.
3. Route the mixer's DVS inputs to Mixxx vinyl control.
4. Confirm both decks track a control record.

That is a working setup on its own. The MIDI mapping then adds pads, loops, and
library navigation on top of something that already functions.

## Effort estimate

| Phase | Est |
|---|---|
| Audio and DVS working | half a day, mostly driver and routing fiddling |
| MIDI log capture, every control | 1 hour, unavoidably manual |
| Faders, EQ, trim, crossfader, pads mapped | 1 day |
| LED feedback and pad mode pages | 1 to 2 days |
| FX section | blocked by firmware, document as a gap |
| Touchscreen | not attempted |

## Sources

- Serato support, RANE SEVENTY-TWO MKII Quickstart Guide (Windows driver requirement)
- Rane Seventy-Two MKII user guide (Rane Control Panel, `.r72` settings files)
- DJ TechTools review, Aug 2020 (VDJ full mapping plus waveform displays)
- DJ Times and B&H, Aug 2020 (MKII opens to Traktor and VirtualDJ)
- VirtualDJ forum thread on Seventy-Two effects (FX ASSIGN not sent, firmware
  tailored to Serato)
- Mixxx forum, "Rane Seventy-two waveform display" thread, 2022 (unanswered)
