# Resolume fleet crossconnect (visual pizzazz)

**Goal:** mixxxxx stays **AV clock + base video**; **Resolume** adds layers, clips, and club FX;
**fleet MCPs** drive both without duplicating wrong OSC addresses.

**Written:** 2026-09-12  
**Status:** plan (partial wiring exists today)

---

## Stack roles

```
mixxxxx (audio + crossfader video)
    │ NDI® (~30 fps) ──────────────────► Resolume layer 1 (base mix)
    │ OSC 11118/11119 ◄──────────────── mixx-dj-mcp (decks, FX, video COs)
    │
    └── future: VJ OSC (TODO 28) ─────► TouchDesigner / Magic / osc-mcp consumers

Resolume (:7000 OSC in)
    ◄── resolume-mcp (clip_control, layer_control, performance_control)
    ◄── osc-mcp resolume_manager (generic fleet OSC hub)
    ◄── mixx-dj-mcp mixx_daw resolume_sync / visuals_* (needs alignment — see gaps)
```

| Repo | Port (fleet) | Job |
|------|----------------|-----|
| **mixxxxx** | 11118/11119 | DJ + deck video; optional NDI sender |
| **mixx-dj-mcp** | 11116 API, 11117 webapp | Cockpit; OSC to mixxxxx; thin Resolume bridge today |
| **resolume-mcp** | 11176 | First-class Resolume OSC (`utils/resolume_osc.py`) |
| **osc-mcp** | (stdio / app-specific) | `resolume_manager` + `resolume-expert` skill; fixed addresses vs old bugs |

**NDI** = video pixels. **OSC** = tempo, opacity, clip triggers, effect params. You want **both**
for “pizzazz”: NDI carries the mix; Resolume carries **your** generative clips and mapping.

---

## What works today

| Link | Status | Note |
|------|--------|------|
| mixxxxx → Resolume **video** (NDI) | Partial | Enable NDI; verify in Studio Monitor — `docs/NDI.md` |
| mixx-dj-mcp → mixxxxx OSC | Works | Deck BPM, play, video COs when fork running |
| mixx-dj-mcp → Resolume (`mixx_daw`) | **Runs, wrong result** risk | Uses legacy paths like `/composition/tempo`, `/layer1/opacity` — not Resolume’s shipped list |
| resolume-mcp → Resolume | Works | Canonical-style paths in `resolume_osc.py` |
| osc-mcp `resolume_manager` | Works | `/composition/tempocontroller/tempo`, layer video opacity — see `skills/resolume-expert` |

Help tab in mixx-dj-mcp already describes the rig (`/help` → Resolume). Orchestrator summary:
`docs/ORCHESTRATOR.md`.

---

## Target architecture (agent-friendly)

1. **Single OSC authority for Resolume** — `resolume-mcp` (or a shared `resolume_osc` package copied
   from there). mixx-dj-mcp should **call resolume-mcp tools** over HTTP/MCP, not invent addresses.  
2. **mixxxxx VJ OSC (TODO 28 phase A)** — publish `beat_distance`, `bpm`, `play`, `crossfader` at
   ~50 Hz from `OscServer` for **any** consumer (Resolume, osc-mcp, TouchDesigner).  
3. **Cockpit “visual rig” preset** — one button: enable NDI on mixxxxx + `performance_control(bpm)`
   + optional `batch_update` for layer opacity from deck energy.  
4. **Transition hooks** — on `mixx_transition` / crossfader events, `clip_control(trigger)` on
   resolume-mcp (strobe column, drop clip, etc.).

---

## TODO (ordered)

### R1 — Stop sending fake OSC (mixx-dj-mcp)

- [ ] **R1.1** Audit `tools/daw.py` `resolume_sync`, `visuals_connect`, `visuals_trigger` against
  `resolume-mcp/src/resolume_mcp/utils/resolume_osc.py` and osc-mcp `resolume-expert` SKILL.  
- [ ] **R1.2** Replace direct `python-osc` with HTTP to resolume-mcp (`/api/v1/tools/call` pattern)
  or import shared address constants.  
- [ ] **R1.3** Mark STATUS: Works only after Resolume demo confirms BPM + opacity move.

### R2 — Deck → Resolume sync loop (mixx-dj-mcp)

- [ ] **R2.1** Background task: poll OSC bridge BPM/play/volume every 100–250 ms →
  `performance_control(bpm)` + `batch_update` layer opacities (energy from volume × pregain).  
- [ ] **R2.2** Cockpit toggle “Resolume follow deck A” (env: `RESOLUME_MCP_URL=http://127.0.0.1:11176`).  
- [ ] **R2.3** Document Resolume composition template (which layer = NDI input, which = FX).

### R3 — mixxxxx clock export (fork)

- [ ] **R3.1** Implement `docs/vj-integration-spout-osc.md` phase A on existing `OscServer`.  
- [ ] **R3.2** Optional: osc-mcp subscribes to mixxxxx VJ OSC for `music_orchestrator` recipes.

### R4 — Agent recipes (fleet)

- [ ] **R4.1** mcp-central-docs or mixx-dj-mcp: “drop build” = mixx_deck load + NDI on +
  resolume-mcp clip trigger.  
- [ ] **R4.2** Prefer **resolume-mcp** for clip/layer/effect; use **osc-mcp** when mixing Resolume
  with Ableton/TD in one `music_orchestrator` flow.

### R5 — Verify on hardware

- [ ] **R5.1** Resolume demo + NDI Tools + mixxxxx MixxxxxVideo skin — one recorded 5 min set.  
- [ ] **R5.2** Update `docs/ORCHESTRATOR.md` integration table from **Works** / **Partial** honestly.

---

## OSC address note (version drift)

Resolume’s official list ships with the app (`OSC list.txt`). Fleet repos currently use:

| Source | BPM-like address |
|--------|------------------|
| osc-mcp (fixed) | `/composition/tempocontroller/tempo` |
| resolume-mcp | `/composition/tempomap/bpm` |

**Before R1 ships:** capture one `OSC list.txt` from **your** Resolume build and pick one module
as canonical; add a one-line version pin in resolume-mcp + mixx-dj-mcp docs.

---

## Related

- `docs/ORCHESTRATOR.md`, `docs/NDI-TARGETS.md`, `docs/vj-integration-spout-osc.md`  
- mixx-dj-mcp `docs/AUDIO_REACTIVE_VISUALS.md`, Help → Resolume  
- `osc-mcp/skills/resolume-expert/SKILL.md`  
- `mcp-central-docs/operations/WEBAPP_PORTS.md` — resolume-mcp **11176**
