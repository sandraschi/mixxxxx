# Rane Seventy-Two MKII: Support Assessment

Written 2026-07-26. Research and feasibility, no implementation yet.
Task brief for implementation is in `docs/HANDOFF-RANE-MAPPING.md`.

## Target hardware (this fleet)

| Device | Role |
|---|---|
| **Rane Seventy-Two MKII** | Battle mixer, USB audio, USB MIDI, dual DVS inputs |
| **Rane Twelve MKII ×2** | Motorised deck controllers (not traditional vinyl turntables) |

Both deck units must be supported. That is **two problems**, not one mapping file:

1. **Mixer** — faders, EQ, crossfader, pads, library nav (`res/controllers/Rane-Seventy-Two-MKII.*`).
2. **Twelve MKII decks** — platter/transport control; forum WIP exists for Rane Twelve MK2
   (turntable lineage). Treat as a follow-on mapping or HID investigation, not something the
   Seventy-Two XML alone covers.

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

**Audio + deck control first, mixer MIDI second.** MIDI mapping is additive, not blocking.

### Path A — DVS / timecode (fastest path to playing)

If the Twelves (or control vinyl through the 72 phono inputs) output timecode into the
Seventy-Two's DVS channels, Mixxx can control decks with **no mapping file**:

1. Install the Rane Windows driver.
2. Select the mixer as the sound device in Mixxx; confirm WASAPI or ASIO in the dropdown.
3. Route both DVS inputs to Mixxx vinyl control (`VINYLCONTROL=ON` in this build).
4. Confirm both decks track.

That is roughly half a day of routing, versus two to three days of MIDI capture and mapping.
**This is not the same as spinning real vinyl** — with two Twelve MKII units you may be sending
timecode from the deck controllers rather than from turntables. Verify what each Twelve outputs
in your Serato/VDJ profile before assuming phono→DVS wiring.

### Path B — MIDI mapping (additive)

Once audio/deck control works, capture the Seventy-Two USB MIDI (`--controller-debug`) and
build `res/controllers/Rane-Seventy-Two-MKII.*`. VirtualDJ already ships a full Seventy-Two
MKII map — use it as a **reference for bulk import** (see below), not as copy-paste CC numbers
without verification.

### Bulk import from VirtualDJ Pro

VDJ device maps live under the VDJ settings folder (device XML + script fragments per controller).
Planned workflow for this fork:

1. Locate the Seventy-Two MKII map in the VDJ profile on the gig machine.
2. Capture raw MIDI from `--controller-debug` and reconcile against the VDJ map structure.
3. Port verified bindings into Mixxx XML/JS; leave unverified rows in `docs/rane-72-midi-map.md`.

There is **no automatic VDJ→Mixxx mapping converter** yet; bulk import means systematic porting
with the VDJ map as the checklist, not a one-click import.

## Effort estimate

| Phase | Est |
|---|---|
| Audio + DVS (both decks) | half a day, driver and routing |
| Twelve MKII deck support | unknown — verify DVS/HID; forum WIP for Twelve MK2 |
| MIDI log capture, Seventy-Two | 1 hour, manual |
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
