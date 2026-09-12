# Plan: Beatgridding and stems (Dani)

**Stakeholder:** Dani (DJ, she/her) — friend using / evaluating mixxxxx + mixx-dj-mcp.  
**Written:** 2026-09-12  
**Status vocabulary:** Works | Runs, wrong result | Dead | Absent (see `docs/STATUS.md`).

---

## What Dani asked

1. **Beatgridding** — marking beat positions in tracks so sync, quantize, loops, and cues line up.  
2. **Realtime stemming** — splitting vocals/drums/bass/instrumental **during playback** (Serato Stems–class), not offline prep.

---

## Honest answers (today)

| Ask | Today | Notes |
|-----|--------|--------|
| Beatgridding / sync / quantize | **Works** (Mixxx core) | Analyze library; beat grid drives engine + fork video FX. |
| Manual grid edit / Serato grid import | **Works** (upstream paths) | See `docs/SERATO-IMPORT.md`; quality = analysis + tags. |
| Realtime AI stems in the app | **Absent** | No gig-safe in-engine separation in default build. |
| Offline stems (Demucs) via MCP | **Works** (prep workflow) | `mixx_stems` separate → WAVs → sampler load via OSC. |
| Play pre-made stem files with per-stem faders | **Not in fork yet** | Upstream **Mixxx 2.6 beta**; merge via `docs/UPSTREAM-2.6-MERGE.md` (#34). |

**One-liner for Dani:** *Grids and sync — yes, analyze your library like any DJ app. Live “remove vocals now” like Serato Stems — not yet; we can prep stems overnight or after upstream 2.6 merge use stem **files** you bring.*

---

## Goals (ordered)

### G1 — Dani can gig on grids without surprises (now)

- [ ] **D1.1** Short “first gig” checklist: analyze crate, test sync on 2 tracks, confirm quantize/loops.  
- [ ] **D1.2** Document where beat grid shows (waveform, sync button, beat FX COs on video skin).  
- [ ] **D1.3** If she uses Serato libraries: run through `docs/SERATO-IMPORT.md` once; note any grid/cue gaps in `docs/rane-72-midi-map.md` style table if needed.

### G2 — Honest stem story (prep + roadmap)

- [ ] **D2.1** Help / Cockpit copy: “offline Demucs prep” vs “realtime stems (not supported)”.  
- [ ] **D2.2** Optional one-page workflow: crate → `mixx_stems(separate)` → load samplers → `stem_swap` transition experiment.  
- [ ] **D2.3** After upstream 2.6 merge: verify stem **playback** COs; update `docs/STATUS.md` and tell Dani what file format she needs pre-gig.

### G3 — Realtime stems (only if Dani will test on hardware)

- [ ] **D3.1** Re-read `docs/STEMS-ONNX-PROBE.md`; decide ONNX in installer vs optional component.  
- [ ] **D3.2** Latency budget + CPU targets (document before coding).  
- [ ] **D3.3** Wire `StemSeparator` CO + OSC; MCP `mixx_stems` mode flag `realtime` only when **Works** in STATUS.  
- [ ] **D3.4** Gig test with Dani: fail loudly if model missing or buffer underruns.

**Do not** mark realtime stems Works from intent alone (handoff rule in `docs/HANDOFF-RANE-MAPPING.md`).

---

## Repo ownership

| Work | Primary repo |
|------|----------------|
| Beat grid, analyze, sync, quantize | mixxxxx (upstream engine) |
| Beat-locked video FX using grid | mixxxxx `src/video/` |
| Demucs prep, sampler OSC, Help text | mixx-dj-mcp |
| Stem file playback (post-2.6) | mixxxxx after merge #34 |
| Realtime ONNX separation | mixxxxx `src/stems/` + MCP exposure |

---

## Suggested timeline

| When | Focus |
|------|--------|
| **This week** | G1 checklist + G2.1 Help honesty (no new engine code required). |
| **After 2.6 merge** | G2.3 stem file playback for Dani if she provides stem exports. |
| **Later / optional** | G3 only with Dani as hardware tester and explicit latency acceptance. |

---

## Related docs

- `docs/STATUS.md` — stem row, video beat FX  
- `docs/SERATO-IMPORT.md` — crates, tags, analysis  
- `docs/STEMS-ONNX-PROBE.md` — ONNX build probe  
- `docs/UPSTREAM-2.6-MERGE.md` — upstream stem **playback**  
- mixx-dj-mcp `docs/TODO.md` — MCP stem UX items  
- mixx-dj-mcp `tools/stems.py` — Demucs portmanteau  

---

## Open questions for Dani

1. Does she need **live vocal removal** every set, or **occasional acapella** (prep acceptable)?  
2. Source library: Serato, Rekordbox, or files only?  
3. Machine spec for any realtime stem experiment (CPU, live vs home practice)?
