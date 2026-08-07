# Serato crate import (mixxxxx)

Use Serato DJ Pro / Studio **prep crates** inside mixxxxx without replacing Serato's database.

## What works today

| Method | Input | Output |
|--------|-------|--------|
| **CLI** | `.crate`, `.vdjfolder`, M3U/PLS/CSV | Mixxx crate in `mixxxdb.sqlite` |
| **GUI** | Serato sidebar (Library → Serato) | Browse Serato `_Serato_` databases on disk |
| **Export back** | `--export-crate --export-format serato` | Serato `.crate` in `_Serato_/Subcrates/` |

Parser tests: `src/test/importcli_test.cpp` (`ImportCliTest.ParseSeratoCrateLocations`).

## Goliath / dev machine without Serato

Serato DJ does **not** need to be installed — only the `_Serato_` folder tree matters for import.

On **goliath** without Serato:

- `GET http://127.0.0.1:11116/api/library/serato/status` → `serato_installed: false`, empty crates
- Unit tests use **synthetic** `.crate` files (`ImportCliTest`, `test_serato_paths.py`)
- When Dani exports a crate from Studio/DJ Pro (USB or sync), drop it in:

```
%USERPROFILE%\Music\_Serato_\Subcrates\
```

  or set `SERATO_SUBCRATES_DIR` for mixx-dj-mcp, then `--import-crate`.

## Windows paths (Dani / Serato DJ Pro)

Serato stores crates here by default:

```
%USERPROFILE%\Music\_Serato_\Subcrates\*.crate
```

Examples:

```
C:\Users\Dani\Music\_Serato_\Subcrates\Warmup.crate
C:\Users\Dani\Music\_Serato_\Subcrates\Peak Hour.crate
```

Full library database (read-only browse in GUI — **not** imported wholesale via CLI):

```
%USERPROFILE%\Music\_Serato_\database V2
%USERPROFILE%\Music\_Serato_\History\...
```

USB / external drive (same layout):

```
E:\_Serato_\Subcrates\Gig.crate
```

## CLI workflow (Studio → mixxxxx)

After editing in **Serato Studio**, crates appear under `_Serato_/Subcrates/`. Import into mixxxxx:

```powershell
cd D:\Dev\repos\mixxxxx\build
.\mixxx.exe --import-crate "$env:USERPROFILE\Music\_Serato_\Subcrates\Warmup.crate" `
  --into-crate "Warmup (from Serato)"
```

Tracks must **exist on disk** at the paths stored in the crate (Serato UTF-16 paths). Missing files are skipped with a log line.

Round-trip export:

```powershell
.\mixxx.exe --export-crate "Warmup (from Serato)" `
  --export-format serato `
  --export-path "$env:USERPROFILE\Music\_Serato_\Subcrates"
```

## GUI workflow

1. **Preferences → Library → Music directories** — include folders where Serato points (often `~\Music`).
2. **Library sidebar → Serato** — scans `_Serato_` trees (see `src/library/serato/seratofeature.cpp`).
3. Mixxx crates — drag or import via CLI for a fixed gig list.

## mixx-dj-mcp + Plex (parallel to Serato library)

Serato integration is **inside mixxxxx**. **Plex** is **mixx-dj-mcp** — search/stream metadata, resolve `plex:rating_key` to a local path for deck load. See mixx-dj-mcp Library page (`PLEX_MCP_URL`, default `http://127.0.0.1:10740`).

Typical Dani stack:

| Tool | Role |
|------|------|
| Serato Studio | Stems edits, mashups, production |
| Serato DJ Pro | Serato-native gigs |
| mixxxxx + mixx-dj-mcp | Video fork, OSC/MCP, Plex-backed search |
| Plex | Owned media library / NAS — not Tidal |

## Limitations

- CLI import reads **crate file track lists**, not the full Serato DB schema.
- Cue points / beatgrids in Serato tags are imported when tracks are analyzed in Mixxx (Serato tag readers exist under `src/track/serato/`).
- Paths in crates must match Windows drive letters Serato wrote (no automatic relinking).

## Tests

```powershell
.\mixxx-test.exe --gtest_filter=ImportCliTest.*
```

Expected: 3 passing tests including synthetic Serato crate with two track paths.
