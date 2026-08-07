# Mixxxxx as AV orchestrator

Mixxxxx is not just a DJ app with video bolted on. In this fleet, it is the **live AV hub**:
audio mixing, deck video, crossfader compositing, export, and **network handoff** to the rest
of your rig — while **mixx-dj-mcp** (and other MCP servers) automate the surrounding tools.

If you have never heard of **NDI** until yesterday, that is normal. NDI is the **video pipe**
that lets other apps subscribe to mixxxxx’s output over Ethernet/Wi‑Fi, the same way OSC is the
**control pipe** for automation.

---

## The stack (one sentence)

**Mixxxxx mixes AV → NDI® sends video to the network → Resolume / OBS / vMix consume it →
mixx-dj-mcp coordinates everything via OSC and MCP.**

---

## Roles

| Piece | Job | Analogue |
|-------|-----|----------|
| **mixxxxx** | Decks, crossfader, video blend, optional NDI sender | The “engine room” |
| **NDI runtime** (user-installed) | Carries blended video on the LAN | HDMI cable, but over the network |
| **OBS** | Stream/recording, overlays, chat | Broadcast desk |
| **Resolume Avenue/Arena** | Club VJ layers, effects, projection | Visual mixer |
| **mixx-dj-mcp** | Webapp + MCP; OSC to mixxxxx; bridges to Plex, SFX, Resolume | Control tower |
| **resolume-mcp** | MCP tools for Resolume clip/layer/effect control | Resolume’s AI remote |

Mixxxxx **does not replace** Resolume or OBS. It **feeds** them clean video and stays the
**source of truth for audio + deck sync**, while specialists handle streaming (OBS) or
layered visuals (Resolume).

---

## Two pipes: control vs video

```
                    ┌─────────────────────────────────────┐
                    │           mixx-dj-mcp               │
                    │  MCP · webapp · library · SFX · …   │
                    └──────────────┬──────────────────────┘
                                   │ OSC UDP 11119 → 11118
                                   ▼
┌──────────────┐   NDI® (LAN)   ┌──────────────┐   stream   ┌─────┐
│   mixxxxx    │ ─────────────► │  Resolume    │ ─────────► │ OBS │ → Kick / YouTube
│ audio+video  │                │  (layers FX) │            └─────┘
└──────────────┘                └──────────────┘
       │                               ▲
       │         same machine          │ OSC :7000 (resolume-mcp)
       └───────────────────────────────┘
```

- **OSC** — “change deck volume”, “enable NDI”, “sync BPM to Resolume”. Small messages, low latency.
- **NDI** — full video frames at ~30 fps. Use **wired gigabit** for 1080p if you can.

You can run without NDI (second monitor HDMI or window capture), but NDI is the **pro, repeatable**
path — especially when OBS and mixxxxx are on the same PC or different PCs on the LAN.

---

## Who this is for

**Model user:** mobile/club DJ who wants one laptop to DJ **with** video, hand off visuals to
Resolume, and stream via OBS — without becoming a broadcast engineer.

**You do not need to master NDI theory.** Install the [free NDI redistributable](https://ndi.link/NDIRedistV5),
enable `[Ndi],enabled` or `--ndi-enable`, open **NDI Studio Monitor** (free) to confirm the
feed, then add the source in Resolume or OBS.

---

## Fleet integration status

| Link | Status |
|------|--------|
| mixxxxx video + crossfader | Works |
| mixxxxx OSC | MVP (11118/11119) |
| mixxxxx NDI sender | Partial MVP — see [`NDI.md`](NDI.md) |
| mixx-dj-mcp → mixxxxx | Works |
| mixx-dj-mcp → Resolume OSC (`resolume_sync`) | Partial — see mixx-dj-mcp `docs/AUDIO_REACTIVE_VISUALS.md` |
| resolume-mcp | Separate repo; needs licensed or demo Resolume |
| mixxxxx → Resolume **video** via NDI | Planned rig; verify with your Resolume + NDI Tools version |

---

## Related docs

- [`NDI-TARGETS.md`](NDI-TARGETS.md) — OBS, Resolume, vMix, and other NDI consumers
- [`NDI.md`](NDI.md) — setup, consumers, troubleshooting
- [`NDI-LICENSING.md`](NDI-LICENSING.md) — GPL-safe dynamic load (for contributors)
- [mixx-dj-mcp Help](https://github.com/sandraschi/mixx-dj-mcp) — webapp `/help` tabs
- mixx-dj-mcp [`docs/AUDIO_REACTIVE_VISUALS.md`](https://github.com/sandraschi/mixx-dj-mcp/blob/main/docs/AUDIO_REACTIVE_VISUALS.md)
