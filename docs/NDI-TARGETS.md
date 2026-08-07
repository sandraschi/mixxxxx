# NDI targets — where mixxxxx video goes

**Context:** mixxxxx is an **NDI sender** (publisher). This doc lists the important **targets** —
apps and tools that **receive** NDI — plus a few related pieces (bridges, monitors) you will
actually encounter in a DJ / stream / club rig.

**You:** never heard of NDI until yesterday → start with [Free verification](#free-verification)
then pick one path from [Quick picker](#quick-picker-for-mixxxxx-users).

Related: [`NDI.md`](NDI.md) (setup) · [`ORCHESTRATOR.md`](ORCHESTRATOR.md) (full stack)

---

## Quick picker for mixxxxx users

| Your goal | Start here | Cost |
|-----------|------------|------|
| “Is NDI working?” | [NDI Studio Monitor](#ndi-studio-monitor) | Free |
| Stream to Kick/Twitch/YouTube | [OBS + DistroAV](#obs-studio--distroav-obs-ndi) | Free (+ optional plugins) |
| Club / mobile VJ layers | [Resolume](#resolume-avenue--arena) | ~€299+ (demo free) |
| Broadcast-style switching | [vMix](#vmix) / [Wirecast](#wirecast) | Paid |
| Sunday stream + MCP automation | OBS + [obs-mcp](https://github.com/sandraschi/obs-mcp) fleet | Free–paid |
| Projections + generative visuals | [TouchDesigner](#touchdesigner) / Resolume | Paid |

---

## Free verification

### NDI Studio Monitor

| | |
|---|---|
| **Vendor** | Vizrt / NDI |
| **Role** | View any NDI source on the LAN by name |
| **Cost** | Free (part of [NDI Tools](https://ndi.video/tools/)) |
| **Platform** | Windows, macOS |
| **mixxxxx use** | **First test** after enabling `[Ndi],enabled` — if you see `Mixxxxx` here, the sender works |

No streaming, no layers — pure sanity check. Every mixxxxx NDI debug session should start here
before opening OBS or Resolume.

### NDI Tools (suite)

Bundled utilities worth knowing:

| Tool | Purpose |
|------|---------|
| **Studio Monitor** | Watch a source |
| **NDI Scan Converter** | Turn a **desktop/window** into an NDI source (opposite direction from mixxxxx) |
| **Webcam Input** | USB camera → NDI |
| **Access Manager** | Firewall / discovery troubleshooting on corporate LANs |

mixxxxx replaces Scan Converter for **your DJ video mix** — cleaner than capturing a window.

---

## Streaming

### OBS Studio + DistroAV (obs-ndi)

| | |
|---|---|
| **Vendor** | OBS Project + [DistroAV](https://github.com/DistroAV/DistroAV) community plugin |
| **Role** | Live stream/recording; **NDI Source** input pulls mixxxxx feed |
| **Cost** | Free |
| **Platform** | Windows, macOS, Linux |
| **Fleet** | Pair with **obs-mcp** for scene/source automation |

**Typical rig:** mixxxxx (NDI out) → OBS NDI Source → overlays (chat, branding) → RTMP to Kick/Twitch.

**Install:** OBS → add DistroAV plugin → Sources → **NDI Source** → select `Mixxxxx` (or your
`[Ndi],source_name`).

**Note:** Formerly “OBS-NDI”; DistroAV is the maintained fork (same MIT-header + dynamic-load
pattern mixxxxx uses).

### Streamlabs Desktop

Can use NDI via plugins or bridge tools; less common in this fleet than plain OBS. Prefer OBS
for documented MCP paths.

---

## VJ & live visuals

### Resolume Avenue / Arena

| | |
|---|---|
| **Vendor** | Resolume |
| **Role** | Layered visuals, effects, multi-output to projectors |
| **Cost** | Avenue ~€299; Arena higher; [unlimited demo](https://resolume.com/download/) |
| **Platform** | Windows, macOS |
| **Fleet** | **resolume-mcp** + mixx-dj-mcp `resolume_sync` (OSC, port 7000) |

**Typical rig:** mixxxxx (base crossfader video via NDI) → Resolume layer 1 → your clips/effects
on upper layers → projector + optional NDI out to OBS.

**Avenue vs Arena:** Avenue is enough for NDI input, OSC, and most mobile/club setups. Arena adds
advanced mapping, DMX, more outputs.

See mixx-dj-mcp Help → **Resolume** tab.

### TouchDesigner

| | |
|---|---|
| **Vendor** | Derivative |
| **Role** | Node-based real-time visuals; **NDI In** TOP |
| **Cost** | Free non-commercial / paid commercial |
| **mixxxxx use** | Generative/projection rigs; mixxxxx feeds a TD comp instead of Resolume |

Strong when you want custom GLSL/network visuals rather than clip-based VJ decks.

### Millumin

macOS-focused show control / VJ; NDI in/out. Smaller community than Resolume; relevant if you
are on Mac club rigs.

### Modulo Player / Kinetic

Higher-end AV show control (Europe); NDI supported. Overkill for bedroom DJ; appears in installed
club systems.

---

## Broadcast & production switching

### vMix

| | |
|---|---|
| **Vendor** | StudioCoast |
| **Role** | Software vision mixer — NDI in/out, replays, virtual sets |
| **Cost** | Tiered licenses (HD / 4K / etc.) |
| **Platform** | Windows |
| **mixxxxx use** | Treat mixxxxx as one **camera/source** in a multi-cam broadcast |

Common in IMAG, church stream, and mid-tier webcast where one machine switches many inputs.

### Wirecast

Telestream; similar niche to vMix (NDI I/O, streaming). macOS + Windows.

### TriCaster / Vizrt live production

Hardware/software switchers with deep NDI integration. mixxxxx would be one source among many.
Relevant for pro install, not typical mobile DJ.

---

## Conferencing & “make Zoom see it”

NDI does **not** plug directly into Zoom/Teams as a native camera in most setups. Usual pattern:

```
mixxxxx → NDI → OBS or Scan Converter → Virtual Camera → Zoom/Teams/Discord
```

| Target | How NDI fits |
|--------|----------------|
| **Zoom** | OBS Virtual Camera, or dedicated NDI→Virtual Cam tools |
| **Microsoft Teams** | Same; or commercial NDI bridge apps |
| **Discord** | OBS Virtual Camera (no native NDI) |

For DJ sets, prefer **OBS → RTMP** to a platform rather than conferencing unless you are doing a
live Zoom gig.

---

## Infrastructure & WAN

### NDI Bridge

Official Vizrt tool to extend NDI across **WAN** (remote venues, cloud). Not needed for same-room
LAN gigs. Adds latency and IT complexity.

### NDI Remote / KVM-style tools

Various vendor tools for remote monitoring of NDI sources. Useful for split booths (DJ booth vs
FOH).

### Access Manager & discovery

If sources do not appear, check Windows firewall, same subnet, and NDI Access Manager. mDNS/Bonjour
must work on the LAN segment.

---

## Hardware you will hear about (usually not mixxxxx *targets*)

These are often **sources** or **side paths**, not consumers of mixxxxx:

| Device / term | Direction | Note |
|---------------|-----------|------|
| **NDI PTZ cameras** | Camera → NDI → switcher/OBS | mixxxxx does not ingest NDI yet |
| **ATEM Mini Pro ISO** | Can **output** NDI | Blackmagic ecosystem |
| **NDI HX** | Compressed camera variant | **Advanced SDK** — mixxxxx deliberately does **not** use HX (GPL) |
| **HDMI → NDI boxes** (e.g. some converters) | HDMI in → NDI out | Alternative to software send |

mixxxxx goal: **software sender** from the crossfader blend — no extra box if the laptop handles it.

---

## Fleet MCP map (automation, not NDI)

MCP servers automate **control**; NDI carries **video**. They complement each other:

| MCP / app | Connects via | NDI role |
|-----------|--------------|----------|
| **mixx-dj-mcp** | OSC → mixxxxx | Indirect — enable video/NDI COs when exposed |
| **obs-mcp** | OBS WebSocket | OBS consumes mixxxxx NDI source |
| **resolume-mcp** | OSC → Resolume | Resolume consumes mixxxxx NDI source |
| **mixxxxx** | NDI publish | **Sender** — source name e.g. `Mixxxxx` |

---

## Comparison matrix

| Target | Best for | NDI input | Typical cost | mixxxxx fleet priority |
|--------|----------|-----------|--------------|------------------------|
| NDI Studio Monitor | Verify feed | Yes | Free | **P0 — always** |
| OBS + DistroAV | Streaming | Yes | Free | **P0** |
| Resolume Avenue | VJ layers | Yes | ~€299 | **P1** |
| Resolume Arena | Mapping / big clubs | Yes | €€€ | P2 |
| vMix | Broadcast switch | Yes | €€ | P2 |
| TouchDesigner | Custom visuals | Yes | Free–€€ | P3 |
| Wirecast | Broadcast | Yes | €€ | P3 |
| Zoom/Teams | Meeting | Via bridge | Varies | P3 (niche) |
| NDI Bridge | Remote site | Relay | Free tier | P4 |
| vMix / TriCaster | Install market | Yes | €€€€ | Awareness only |

---

## Network expectations

| Resolution / fps | LAN guidance |
|------------------|--------------|
| 720p30 | Wi‑Fi often OK for tests |
| 1080p30 | **Gigabit wired** recommended for live gigs |
| 1080p60+ | Wired + dedicated NIC; watch CPU on sender (mixxxxx) |

mixxxxx sends ~720p/1080p letterboxed BGRA from `VideoMixer` (~30 fps). Multiple subscribers
(OBS + Resolume + Monitor) can attach to the **same** source name.

---

## What mixxxxx does *not* target (today)

- **NDI receiver / camera ingest** — send-only MVP
- **NDI HX encode** — commercial Advanced SDK; excluded for GPL
- **Audio-over-NDI** — video-only sender; audio stays in mixxxxx / PA / OBS separately
- **Bundled runtime** — user installs [NDI redistributable](https://ndi.link/NDIRedistV5)

---

## References

- [NDI Tools download](https://ndi.video/tools/)
- [DistroAV (OBS NDI)](https://github.com/DistroAV/DistroAV)
- [Resolume](https://resolume.com/)
- [vMix NDI](https://www.vmix.com/help27/NDI.aspx)
- mixxxxx [`NDI.md`](NDI.md) · [`ORCHESTRATOR.md`](ORCHESTRATOR.md)
- mixx-dj-mcp Help → **NDI** / **Resolume** / **AV Rig**
