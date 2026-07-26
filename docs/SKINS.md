# Mixxx / mixxxxx skins

## No VDJ-style marketplace

VirtualDJ ships device maps in-app and on their site. **Mixxx has no central skin store.**
Distribution is forum threads, GitHub repos, and SourceForge zips — manual install.

## Install any community skin

1. Close Mixxx.
2. Download and unzip so `skin.xml` is at the folder root.
3. Copy the folder to `%LOCALAPPDATA%\Mixxx\skins\` (create `skins` if missing).
4. Restart → **Preferences → Interface → Skin**.

Built-in skins live in `mixxxxx/res/skins/` and do not need copying.

## Where to find skins

| Source | URL | Notes |
|---|---|---|
| Mixxx Discourse (Skins category) | https://mixxx.discourse.group/c/skins/7 | Primary community hub; search "skin" |
| esbrandt/mixxx-skins | https://github.com/esbrandt/mixxx-skins | **SVG source layers** for LateNight, Phoney, Outline — mod starting point, not ready-to-run zips |
| djraw/Traktmixxx-RAW | https://github.com/djraw/Traktmixxx-RAW | Traktor-like 4-deck skin (check mixxxxx version compat) |
| Dark Metal (legacy) | https://sourceforge.net/projects/dark-metal-mixxx-skin/ | Old but complete; forum-linked |
| Mixxx wiki | https://github.com/mixxxdj/mixxx/wiki/Creating-Skins | Author docs, install steps |
| mixx-dj-mcp manifest | `mixx_skin(operation="list")` | Curated pointers only; most entries are bundled |

Bright/daylight skins are rare. Bundled **Shade → Summer Sunset** scheme is the closest stock option.
**Mixxxxx Video → Daylight** scheme targets outdoor use (see `res/skins/MixxxxxVideo/`).

## Mixxxxx Video skin

| File | Role |
|---|---|
| `res/skins/MixxxxxVideo/skin.xml` | Full LateNight layout; video defaults; Daylight scheme |
| `res/skins/MixxxxxVideo/style_daylight.qss` | Light QSS (PaleMoon derivative) |
| LateNight assets | Shared via `skins:LateNight/...` URLs — no duplicate SVG tree |

Regenerate daylight QSS from PaleMoon:

```powershell
Copy-Item "D:\Dev\repos\mixxxxx\res\skins\LateNight\style_palemoon.qss" "D:\Dev\repos\mixxxxx\res\skins\MixxxxxVideo\style_daylight.qss"
# then run color replacements (see git history) or edit in Inkscape + QSS editor
```

**Inkscape / inkscape-mcp:** edit `LateNight/palemoon/style/*.svg` or esbrandt layered sources;
QSS handles panel backgrounds, SVG handles knobs/buttons.

## mixx-dj-mcp skin tools

```
mixx_skin("list")
mixx_skin("create_video_skin")          # → %LOCALAPPDATA%\Mixxx\skins\MixxxxxVideo
mixx_skin("create_skin", name="...", prompt="bright daylight white panels")
```

Video requires **legacy skin** (not QML). `VideoWidget` is absent under QML path.
