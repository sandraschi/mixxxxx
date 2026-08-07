# Mixxxxx skins

## Bundled video skin

**Mixxxxx Video** lives at `res/skins/MixxxxxVideo/`. It is a thin skin:

- `skin.xml` — LateNight layout with video preview/output defaults
- `style_daylight.qss` — optional **Daylight** color scheme (Preferences → Interface → Skin → color scheme)
- SVG/assets — shared with LateNight (`skins:LateNight/...` references)

Install to user folder via mixx-dj-mcp: **Skins → Install Mixxxxx Video** (copies to `%LOCALAPPDATA%\Mixxx\skins\MixxxxxVideo`).

## Color schemes

| Scheme | File | Notes |
|--------|------|-------|
| PaleMoon | LateNight default QSS | Dark club |
| Classic | LateNight | Dark |
| Daylight | `style_daylight.qss` | Outdoor / bright rooms — **v2** (light waveform wells, Jul 2026) |

Current Daylight v2 removes PaleMoon dark waveform/toolbar wells. Install via mixx-dj-mcp **Skins** page; schemes live in `mixx-dj-mcp` → `docs/SKINMAKER.md`.

## mixx-dj-mcp

- Webapp **Skins** page: `GET /api/skins` manifest
- MCP tool: `mixx_skin` — `list`, `create_video_skin`, `create_skin` (LLM + inkscape-mcp)

## Authoring new skins

See **mixx-dj-mcp** [`docs/SKINMAKER.md`](../../mixx-dj-mcp/docs/SKINMAKER.md) — skinmaker is integrated there (MCP + Skins webapp), not a separate repo.
