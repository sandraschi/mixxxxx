# Upstream Mixxx philosophy (why the fork exists)

**Audience:** You, Dani, future agents — not a flame thread on Mixxx forums.  
**Written:** 2026-09-12

---

## What Mixxx upstream optimizes for

The [mixxxdj/mixxx](https://github.com/mixxxdj/mixxx) project is a **volunteer-maintained,
GPL DJ application** used in real gigs. Their implicit contract:

| Priority | Meaning |
|----------|---------|
| **Gig stability** | Crashes mid-set are unacceptable; betas are labeled “not for live use.” |
| **Hardware mappings** | Hundreds of controllers; each mapping is maintenance forever. |
| **Audio engine correctness** | Latency, sync, vinyl control — hard C++, slow to change. |
| **Conservative features** | No video mixing ([#8034](https://github.com/mixxxdj/mixxx/issues/8034) open years). Stable release **2.5.6**; **2.6** still beta. |

That is not “we hate AI.” It is **different success metrics** than a fleet fork with one
owner, agentic tooling, and a willingness to ship video + OSC + NDI on branch `video`.

---

## What upstream is slow to ship (by design)

- **Video DJ** — structurally out of scope for core Mixxx today.  
- **NDI / Spout / VJ OSC** — club rig plumbing; niche for upstream reviewers.  
- **Agent/MCP control plane** — outside their problem domain.  
- **“Move fast and vibecode”** — incompatible with unpaid review of real-time audio PRs.

**2.6 beta** adds useful DJ features (stem **file** playback, cue reorder, controllers) but
**not** a substitute for mixxxxx’s video stack. Merge playbook: `docs/UPSTREAM-2.6-MERGE.md`.

---

## What mixxxxx optimizes for

| Layer | Owner | Velocity |
|-------|--------|----------|
| Engine + mappings baseline | Upstream Mixxx | Slow, high quality |
| Video, export fixes, OSC MVP, CLI gig flags | **mixxxxx** | Fast, evidence-based (`docs/STATUS.md`) |
| AI, library, Plex, SFX, Resolume orchestration | **mixx-dj-mcp** + fleet MCPs | Fast, integration-first |

**Rule (same as Rane handoff):** mark **Works** only when traced or tested — upstream slowness
is not an excuse for our own doc fiction.

---

## How to treat upstream

1. **Supplier, not competitor** — merge engine wins when conflict cost is justified (#34).  
2. **Do not wait** for video, NDI, or MCP — that is why `video` branch exists.  
3. **Respect their reviewers** — when you upstream a patch, bring tests and minimal diff; when
   you fork, own the regression matrix.  
4. **Pitch to DJs:** “Mixxx-class reliability + features Serato won’t ship (video, agents).”

---

## Agentic / vibecoding stance (this fleet)

- **Agents belong in the control and integration layer** (MCP, OSC, scripts, webapp) — not
  unreviewed edits to `EngineBuffer` at 2 a.m.  
- **Specs survive context resets** (`PLAN-*.md`, `HANDOFF-*.md`, `UPSTREAM-2.6-MERGE.md`).  
- **Upstream greybeards** ≈ governance + liability + gig trust; **your edge** ≈ orchestration
  speed and AV features they will not prioritize.

---

## Related

- `docs/IDEAS.md` — Serato moat vs velocity on integration layer  
- `docs/ORCHESTRATOR.md` — AV hub + Resolume/OBS  
- `docs/RESOLUME-FLEET-CROSSCONNECT.md` — visual pizzazz via fleet MCPs  
- `docs/UPSTREAM-2.6-MERGE.md` — when to merge `2.6`
