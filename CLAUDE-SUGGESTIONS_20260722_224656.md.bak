# Claude Suggestions — mixxxxx / mixx-dj-mcp

Brainstorm from a Claude chat session, reviewing the repo state as of Sprint 1
(phase indicator + Rekordbox export shipped). Invited to be merged with
DeepSeek's own "most requested issues" pass. Not prescriptive — pick what's
useful, drop the rest.

## Context noted during review

- `smart_crate.py` (mixx-dj-mcp) already does natural-language crate creation:
  prompt → Mixxx search syntax via local Ollama (`llama3.2:3b`), with a
  keyword-matching fallback if Ollama isn't running. This is the seed of the
  whole AI-workflow direction below — worth building on, not duplicating.
- Mixxx's existing analyzer stack (`analyzerbeats`, `analyzerkey`,
  `analyzerwaveform`) already computes BPM, key, and beat grid for every
  library track. Most of the ideas below need no new DSP — just wiring an
  LLM or small model on top of data that's already extracted.
- `src/library/serato/` already exists in the mixxxxx codebase (Serato
  *import* parsing). That's most of the hard part of Serato *export* already
  paid for — see below.

## The one "most amazing" add

**AI-assisted full-set auto-sequencing.** Feed it a crate or folder; it
proposes a full track order using harmonic mixing (Camelot wheel adjacency),
an energy-curve shape (build → peak → cooldown, or whatever arc is
requested), and phrase-aligned transition points — using BPM/key/beat-grid
data Mixxx already computes, plus an LLM reasoning about the *shape* of the
set, not just pairwise compatibility scores.

Not "Auto DJ" (exists everywhere, sounds robotic/rules-based). The pitch is
a set that reads as intentional because something is reasoning about
narrative arc. Neither Serato nor VDJ ships this — their Auto-DJ modes are
both rules-based shuffle-with-constraints. This is winnable specifically
because it's Python/MCP-glue-plus-local-LLM work, which is where this
project is fast, not real-time C++ media-engine work, which is where the
video A/V-sync gap is still genuinely stuck.

## Positioning vs. Serato (be honest about this)

Serato's moat is hardware certification (every major controller ships
Serato-ready) and ~20 years of community controller mappings — not worth
attacking head-on. Their actual weakness is exactly what's already being
exploited here: a legacy C++ codebase with a conservative release cadence,
because their business is selling stability to working DJs mid-gig, not
shipping experimental AI features. That's the same reason they've declined
to add video. The real edge is velocity on the AI-native layer, not parity
on the media-engine layer. Pick fights there.

## Exporters

- **Serato export**: closer than it looks. Import parsing for Serato's
  binary crate/tag format already exists in `src/library/serato/` — writing
  the format back out reuses that structural knowledge instead of starting
  a reverse-engineering effort from zero. Real work, but the expensive part
  (understanding the format) is already done.
- **VirtualDJ export**: the cheap win. `database.xml` is plain, documented
  XML (track nodes, tag attributes), and playlists are `.vdjfolder`/M3U
  files in a known directory layout. No binary format at all. Realistically
  a day of work, not a sprint.
- Rekordbox export is already shipped (Sprint 1) — two of three major
  competitor library formats would be covered once Serato/VDJ land.

## AI workflow ideas, roughly cheap-and-real → ambitious

1. **Semantic crate search beyond keywords** — `smart_crate` currently does
   prompt→search-syntax translation on metadata. Add local audio embeddings
   (CLAP or similar) so "warm sunset chill vibe" matches on actual sonic
   character, not just genre/BPM tags.
2. **Voice control over the OSC bridge** — mixx-dj-mcp already has an OSC
   bridge and deck/mixer tools; local Whisper + tool-calling gets
   "bring in deck two, kill the bass" hands-free mid-mix. Wiring, not
   invention.
3. **AI-suggested transition points** — surface phrase boundaries
   (16/32-bar) from existing beat-grid analysis, with LLM narration of
   *why* a point works, not just where it is.
4. **Stem-aware live transitions** — once stem separation exists (see
   Known Gaps in PRD.md), auto-trigger "drop vocal from deck A over
   instrumental of deck B" as a one-button move instead of a manual
   EQ/fader dance.
5. **Post-set feedback loop** — analyze a recorded set's phase-lock
   consistency, energy curve, and transition timing after the gig and give
   an honest debrief. Nobody ships this because it means admitting
   mistakes exist in the mix — good differentiator, more speculative to
   build than the rest of this list.

## Open architectural question, not resolved here

PRD.md's own Known Gaps table estimates stem separation at "ONNX Runtime
integration, ~400 lines C++" — i.e. running the model in-process rather
than shelling out to Python Demucs. That's a real fork in approach:
in-process ONNX gets tighter integration and no Python runtime dependency
in the shipped exe, at the cost of needing an ONNX-exported Demucs graph
(HTDemucs isn't natively distributed as ONNX, so that's conversion work
up front) versus just calling the Python Demucs package from mixx-dj-mcp,
which is far less work but adds a Python dependency to the pipeline. Worth
deciding deliberately rather than defaulting to whichever gets typed first.
