# Competitive gaps vs Serato / VDJ / Algoriddim

Honest map of what mixxxxx can close vs what stays out of scope.

## Can we fill the gaps?

**Partially — yes, for desktop fleet DJ + video + MCP.** We will not replicate full commercial ecosystems (certified hardware firmware, mobile apps, streaming license deals) inside the fork. We *can* close the gaps that matter for home/studio + gig prep workflows.

| Gap | Feasible? | Path |
|-----|-----------|------|
| **Real-time stems** | Yes (hard) | TODO 31: `-DONNX_RUNTIME=ON`, model download on first use |
| **Library / prep workflow** | Yes (medium) | Fix export labels (TODO 13), crate GUI CO, Serato/VDJ import; mixx-dj-mcp for AI search + Plex |
| **Video at VDJ tier** | Partial | Beat FX, Spout/VJ OSC (TODO 28), NDI (TODO 27) — not full shader marketplace |
| **Pro hardware certainty** | Partial | Shelf-test mappings (TODO 32); no OEM firmware deals |
| **Mobile (djay class)** | No (fork scope) | Separate product |
| **Streaming (Tidal/Apple)** | Unlikely | Licensing; **Plex via mixx-dj-mcp** for owned media — see below |

## Streaming: Tidal vs Plex (what we can actually do)

**Tidal in Serato / djay / VDJ** is a **paid partnership**, not a hackable API:

- Vendor app embeds Tidal OAuth/SDK; audio streams under Tidal ToS (no WAV export, offline caps).
- Requires Tidal + DJ software subscriptions where applicable.
- Catalog lives on Tidal servers — not your Plex NAS.

**mixxxxx has no Tidal deal** (and GPL + licensing make one unlikely). We cannot officially "filch" Tidal playback without the same contract Serato signed.

**Our substitute — Plex integration (mixx-dj-mcp):**

```
Webapp / Library → mixx-dj-mcp :11116 → plex-mcp :10740 → Plex Media Server → files on disk
```

- Intelligent search, filters, semantic mode (`PLEX_MCP_URL` in `.env`).
- `plex:rating_key` resolved to a local path for deck load when the file is reachable.
- You **own** the files (rip, purchase, CD ingest) — not a 60M-track streaming catalog.

**For Dani:** Serato+Tidal for Serato-native gigs; Plex+mixxxxx for owned library, video fork, and MCP.

## Hardware firmware (Serato / VDJ coops)

OEM **firmware modes** (Pioneer "Serato", Rane, etc.) are **proprietary and certified**. Serato/VDJ ship with vendor blessing; displays, motor control, and hidden DVS live there.

**We cannot officially replicate that** without manufacturer partnership. Community MIDI/HID XML maps yes; cloning firmware protocols or leaking blobs — **no** (legal + fleet policy).

Honest path: generic MIDI, OSC via mixx-dj-mcp, shelf-test maps (TODO 32), document "switch controller to MIDI/PCDM mode."

## Serato import + ONNX probe (implemented docs)

- **`docs/SERATO-IMPORT.md`** — `%USERPROFILE%\Music\_Serato_\Subcrates\*.crate`, `--import-crate`
- **`docs/STEMS-ONNX-PROBE.md`** — `just probe-onnx-stems` / TODO 31
- Tests: `ImportCliTest.*` (mixxxxx), `test_serato_paths.py` (mixx-dj-mcp)

## Recommended priority

1. **Stems probe** (TODO 31) — compile test; unlocks mashup prep vs Serato Studio Stems
2. **Serato library import** — upstream reads Serato crates; document + test crate paths
3. **Prep desk in mixx-dj-mcp** — edit markers, intro/outro notes, export to crate (Studio → DJ Pro analogue)
4. **NDI + Spout** — OBS/Resolume handoff
5. **Installer + OSC shortcut** — `docs/INSTALLER.md`

## Serato Studio vs Fairlight vs Reaper

**Serato Studio** is Serato's **beat-making / edit DAW**, not a full linear multitrack like Reaper or Fairlight. It targets DJs who want fast intro/outro trims, mashups, and beats.

| | Serato Studio | Fairlight (DaVinci) | Reaper |
|---|---------------|---------------------|--------|
| **Primary job** | DJ edits, beats, samples | Post/film/broadcast audio | General DAW |
| **Learning curve** | Low (DJ-native) | High (Resolve suite) | Medium-high |
| **Stems** | Built-in (Serato Stems) | Plugins / external | Plugins / scripts |
| **Serato DJ Pro link** | **Ecosystem** | None | None |

### Serato Studio + DJ Pro integration

Separate apps (not run at once), tied by **DJ Suite**:

- **Shared library** — tracks, crates, cue points
- **Hardware** — DJ controllers usable in Studio for edits
- **Workflow** — edit in Studio, perform in DJ Pro
- **Not** a plugin inside DJ Pro — library + licensing + workflow

**Dani using both:** Studio for prep (stems, intros/outros), DJ Pro for gigs. mixxxxx + mixx-dj-mcp can approximate prep if stems + crate import land — not replace Studio's sequencer.

### Fairlight

Usually **DaVinci Resolve's Fairlight page** — editorial/video-house audio. vs Reaper: Fairlight is timeline/ADR inside Resolve; Reaper is scriptable general DAW. Neither shares Serato's DJ library.
