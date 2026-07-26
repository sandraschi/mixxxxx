# HANDOFF: Rane Seventy-Two MKII MIDI mapping

**For:** Cursor / Windsurf / any coding agent picking this up cold.
**Written:** 2026-07-26
**Repo:** `D:\Dev\repos\mixxxxx` (fork of Mixxx 2.5.6, branch `video`)
**Background research:** `docs/RANE-SEVENTY-TWO-MKII.md` (read this first)

---

## 0. Read this before touching anything

This repo has a specific failure history: **documentation written from intent, then
reused as evidence in the next review.** Three separate features were documented as
"Working" while having zero call sites. A whole OSC control table was documented for
a server that does not exist.

So the rule here is narrow and non-negotiable:

> Mark a control as mapped only after you have physically moved it and seen the
> value change in Mixxx. Everything else goes in the "unverified" table.

If you cannot test something because the hardware is not plugged in, say so in your
output. Do not infer. Do not fill in plausible CC numbers. A mapping file full of
guessed MIDI addresses is worse than an empty one, because it looks finished.

---

## 1. What you are building

Two files, following Mixxx's naming convention exactly:

```
res/controllers/Rane-Seventy-Two-MKII.midi.xml
res/controllers/Rane-Seventy-Two-MKII-scripts.js
```

Nothing else. **Do not touch `src/`.** This is a data and glue task, not an engine
change. In particular do not touch `src/video/`, `src/export/`, or
`src/engine/controls/videosynccontrol.cpp`, which were fixed earlier today and are
covered by tests.

---

## 2. Current state

- **No Rane mapping exists** in this tree or upstream Mixxx. 142 mappings present,
  none for any Rane device.
- The mixer **does** emit usable USB MIDI. VirtualDJ has a full mapping of it. This
  is the key de-risking fact, established in the research doc.
- **The FX section will not fully map.** The Seventy-Two firmware was tailored to
  Serato's effects model and does not send FX ASSIGN data to software. Expect to
  document this as a gap rather than solve it.
- **Do not attempt the touchscreen.** Proprietary protocol, weeks of USB captures,
  no guarantee. Out of scope.

---

## 3. Build and run

**Mixxx must be closed before building** or the link fails with
`LNK1104: cannot open file 'mixxx.exe'`.

```powershell
& "D:\Dev\repos\mixxxxx\build\cmake_build.cmd"
```

Note the `&` and the full path. From inside `build\`, `.\cmake_build.cmd` also
works. Bare `build\cmake_build.cmd` will fail, since PowerShell tries to autoload a
module named `build`.

Mapping files are **resources, not compiled code**. Editing the XML or JS does not
require a rebuild. Restart Mixxx, or use the reload button in Controller
Preferences.

---

## 4. Step one: capture what the hardware sends

This is the whole job. Everything downstream is mechanical once you have the log.

```powershell
& "D:\Dev\repos\mixxxxx\build\mixxx.exe" --controller-debug --log-level debug --settings-path "C:\temp\rane_profile"
```

The flag is `--controller-debug`. The old `--controllerDebug` still works but is
hidden and deprecated in 2.5.

Use a throwaway `--settings-path` so the real library is never at risk.

Then, methodically, one control at a time:

1. Move it through its **full range** (for continuous controls, so you capture the
   min and max, and whether it is 7 bit or 14 bit).
2. Press and release it (for buttons, so you capture note-on and note-off, and
   whether it sends a value or a toggle).
3. Record the status byte, CC or note number, and observed value range.

Work in this order and log as you go:

| Group | Controls |
|---|---|
| Channel strip A | trim, hi, mid, low, filter, fader, cue, PFL |
| Channel strip B | same |
| Crossfader | position, plus reverse/contour switches if they send anything |
| Pads deck A | 8 pads, then each pad mode page |
| Pads deck B | same |
| Transport / nav | browse encoder, load, back, shift, view/menu |
| FX section | **expect this to send little or nothing, log it anyway** |

Save the raw log. Do not throw it away after mapping; it is the primary artifact and
the next person needs it.

Write the result to `docs/rane-72-midi-map.md` as a table before writing any XML.
That table is the deliverable of step one and it should be reviewable on its own.

---

## 5. Step two: write the mapping

### Reference material in this tree

- `res/controllers/midi-components-0.0.js` is the standard component library. Use
  it. Do not hand-roll button and pot handling.
- Good exemplars for structure, LED feedback, and shift layers:
  - `Allen-and-Heath-Xone-K2-scripts.js`
  - `Behringer-DDM4000-scripts.js` (mixer rather than controller, closest in shape)
  - `Denon-MC6000MK2-scripts.js` (pad modes and feedback)
- `Reloop Terminal Mix 2-4.js` shows the `midi-components` include pattern.

### Split of responsibilities

**XML** handles the device match block and simple one-to-one bindings: faders to
volume, EQ knobs to their filter COs, crossfader to `[Master],crossfader`.

**JS** handles anything stateful: pad mode pages, the shift layer, LED output.

### Target ControlObjects

Mixxx has no `--dump-controls` flag yet (that is `docs/TODO.md` item 24). Until it
does, use the Developer Tools ControlObject browser to find target names:

```powershell
& "D:\Dev\repos\mixxxxx\build\mixxx.exe" --developer
```

Developer menu, Developer Tools. It is searchable and lets you set values live,
which is also how you verify a binding worked without touching the hardware.

Mixxx's built-in MIDI learning wizard (Controller Preferences, Learning tab) handles
the simple bindings without hand-editing XML. Use it for the trivial 40% and hand
write the rest.

---

## 6. Step three: LED feedback

This is what separates a finished mapping from a half-done one. Pads should reflect
Mixxx state, not just send it.

Output MIDI back to the device on CO `valueChanged`. `midi-components-0.0.js`
handles most of this if you use `components.Button` with `midi:` output specified.

Leave this until inputs are fully working and verified. Do not interleave.

---

## 7. Acceptance criteria

A control counts as done only when all three hold:

1. Moving the physical control changes the expected value in Developer Tools.
2. The reverse, where applicable: changing the CO in Developer Tools moves the LED
   or display state on the hardware.
3. It is listed in the verified table in `docs/rane-72-midi-map.md`.

Ship criteria for a first release:

- [ ] Both channel strips: trim, 3 band EQ, filter, fader, cue
- [ ] Crossfader
- [ ] 16 pads in at least Cue mode and Loop mode
- [ ] Browse encoder plus load buttons
- [ ] Shift layer working
- [ ] LED feedback on pads and cue buttons
- [ ] `docs/rane-72-midi-map.md` complete, including an explicit **unmapped** section
- [ ] FX gap documented with the reason (firmware does not send FX ASSIGN)

---

## 8. Explicitly out of scope

- The touchscreen and waveform display. Proprietary, not worth it.
- Any change under `src/`.
- DVS and timecode setup. That is audio routing and configuration, not a mapping,
  and it is covered in `docs/RANE-SEVENTY-TWO-MKII.md`. It is also the higher value
  path, so if the goal is "get playing tonight", do that first and come back to this.
- Solving the FX section. Document it, do not fight the firmware.

---

## 9. Where to report status

- Mapping table and progress: `docs/rane-72-midi-map.md` (create it)
- Anything that changes the picture in the research doc: update
  `docs/RANE-SEVENTY-TWO-MKII.md`
- Session log entry: append to `docs/PROGRESS-<date>.md`

Use the status vocabulary already established in `docs/STATUS.md`:
**Works** (traced or tested), **Runs, wrong result**, **Dead**, **Absent**. Do not
invent new status words and do not use a bare checkmark.

---

## 10. If the hardware is not connected

Then step one is impossible and the honest output is a scaffold, clearly labelled:

- The two files created, with the device match block filled in from the USB device
  name and a structure ready to receive real values.
- Every binding left as an explicit `TODO` with no invented MIDI address.
- A note in your summary saying the mapping is unverified and why.

That is a legitimate and useful deliverable. A file full of plausible looking
guessed CC numbers is not.
