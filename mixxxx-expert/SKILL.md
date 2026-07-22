# Mixxxxx Expert Skill

You have access to **mixxxxx** — a video-enabled fork of Mixxx 2.5.6 with AI integration.

## What mixxxxx adds over vanilla Mixxx

### Video
- Video playback alongside audio (companion `.mp4`/`.mkv`/`.mov`/`.webm` autoloaded with same basename)
- Hardware-accelerated decode (D3D11VA or CUDA via FFmpeg)
- Crossfader video blending between decks
- Per-deck VFX: brightness, contrast, saturation
- Fullscreen projector output on secondary monitor
- `<VideoWidget>` skin element in LateNight-based skins

### A/V Sync
- VideoSyncControl: sample-accurate deck position drives video frame selection
- 3-tier drift handling: seek on >2s drift, skip frame on moderate drift, throttle when ahead

### Exporters (all wired and working)
- **Rekordbox**: writes Pioneer .pdb format to USB drives (auto-detects FAT32/exFAT drives)
- **Serato**: writes `_Serato_/database V2` + Subcrates
- **VirtualDJ**: writes `database.xml` + `.vdjfolder` playlist files

### Stem Separation (C++ ONNX Runtime, option-gated `-DONNX_RUNTIME=ON`)
- HTDemucs v4 model, auto-downloaded from HuggingFace on first use
- Outputs: vocals.wav, drums.wav, bass.wav, other.wav
- ~2s per minute of audio on GPU, ~21s on CPU

### Phase Indicator
- Neon ring visualization of beat alignment between decks
- Green (locked) → yellow → orange → red (out of phase)
- `<PhaseIndicator>` skin element

## Companion MCP Server

[mixx-dj-mcp](https://github.com/sandraschi/mixx-dj-mcp) provides 12 portmanteau tools (~84 ops):

| Tool | Ops | Purpose |
|------|-----|---------|
| `mixx_deck` | 19 | Transport + video |
| `mixx_library` | 8 | Library search |
| `mixx_effects` | 7 | Effect chains |
| `mixx_mixer` | 8 | Crossfader, EQ |
| `mixx_crate` | 5 | Smart + agentic crates |
| `mixx_stems` | 6 | Demucs separation |
| `mixx_set` | 3 | Sequencing + recording |
| `mixx_skin` | 7 | Skin browser + generator |
| `mixx_vinyl` | 5 | Vinyl catalog + gig pick |
| `mixx_controller` | 5 | Auto-detect |
| `mixx_daw` | 8 | DAW export + Resolume sync |
| `mixx_transition` | 3 | AI transitions |

## OSC Control

mixx-dj-mcp talks to mixxxxx via OSC on ports 11118/11119. The webapp runs on port 11117.

## Key Workflows

- "Load a track to deck 1" → `mixx_deck(operation="load", deck=1, track_path="...")`
- "Play deck 2" → `mixx_deck(operation="play_pause", deck=2)`
- "Apply reverb to deck 1" → `mixx_effects(operation="chain_load", rack=1, unit=1, effect="Reverb")`
- "Enable video for deck 1" → `mixx_deck(operation="video_enable", deck=1, enable=True)`
- "Separate stems for deck 1" → `mixx_stems(operation="separate", deck=1)`
- "Export to Rekordbox USB" → trigger `[Channel1],export_rekordbox` CO
- "What's the phase on deck 1?" → read `[Channel1],phase` CO (0-360°)
- "Sync deck 2 to deck 1" → `mixx_deck(operation="sync_enable", deck=2, enable=True)`
- "Crossfade to deck 2 with AI transition" → `mixx_transition(operation="suggest", deck_a=1, deck_b=2)`
