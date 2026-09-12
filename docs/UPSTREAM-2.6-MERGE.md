# Upstream Mixxx 2.6 merge playbook

**Fork:** mixxxxx `video` (base tag **2.5.6**).  
**Upstream:** `mixxxdj/mixxx` branch **`2.6`** (beta; stable release still 2.5.6).  
**Tracking:** `docs/TODO.md` item **#34**.

Upstream 2.6 adds **per-stem volume/effects**, **hotcue reorder**, and **controller UX**. It does **not** ship video mixing ([#8034](https://github.com/mixxxdj/mixxx/issues/8034)). This fork owns video, NDI, export fixes, OSC MVP, and CLI gig flags.

---

## Before you merge

1. Close Mixxx/mixxxxx (avoid `LNK1104` on `mixxx.exe`).
2. Checkpoint: `git status` clean, note current `video` HEAD.
3. Add upstream if missing:

```powershell
Set-Location D:\Dev\repos\mixxxxx
git remote add upstream https://github.com/mixxxdj/mixxx.git
```

(If `upstream` exists: `git fetch upstream`)

4. Read upstream delta (no merge yet):

```powershell
git log --oneline HEAD..upstream/2.6 --max-count=30
git diff --stat HEAD...upstream/2.6
```

---

## Merge

```powershell
Set-Location D:\Dev\repos\mixxxxx
git checkout video
git merge upstream/2.6 -m "merge: upstream Mixxx 2.6 beta into video"
```

If conflicts are heavy, prefer **`git merge --no-commit upstream/2.6`**, resolve, then commit.

### Conflict priorities (keep mixxxxx)

| Area | Rule |
|------|------|
| `src/video/**` | **Ours** — entire tree is fork-only |
| `src/control/oscserver.*` | **Ours** unless upstream adds generic OSC; port shared fixes manually |
| `src/export/**`, fork export CO wiring | **Ours** unless upstream fixes apply cleanly |
| `CMakeLists.txt` / `FFMPEG` / `NDI` options | Merge carefully; preserve `NDI`, video tests, `if(FFMPEG)` blocks |
| `res/skins/MixxxxxVideo/**` | **Ours** |
| Engine / stems / cues / controllers | **Theirs** when clearly upstream 2.6 feature work |
| `res/controllers/**` | **Theirs** for upstream mapping fixes; do not drop Rane WIP files |

After merge, grep for duplicate CO names or removed `ConfigKey`s in video and export paths.

---

## Build and test

```powershell
& "D:\Dev\repos\mixxxxx\build\cmake_build.cmd"
& "D:\Dev\repos\mixxxxx\build\mixxx-test.exe" --gtest_filter=VideoMixerTest.*
& "D:\Dev\repos\mixxxxx\build\mixxx-test.exe" --gtest_filter=OscServerTest.*
```

Optional broader gate:

```powershell
& "D:\Dev\repos\mixxxxx\build\mixxx-test.exe"
```

(Expect 4 legacy `ControllerScriptEngineLegacyTimer` failures unless upstream fixed them.)

---

## Manual smoke (15 min)

1. Launch with MixxxxxVideo skin; two decks with companion video.
2. `[Skin],show_video_output=1`; sweep crossfader; confirm master video blend.
3. OSC: `mixx-dj-mcp` health shows `mixxx_osc: true` with mixxxxx running.
4. If stem UI changed: load a track with stem files; confirm fork ONNX path still behaves or document regression in `docs/STATUS.md`.

---

## After merge

1. Update `docs/STATUS.md` — what landed from upstream vs still fork-only.
2. Update `CHANGELOG.md` under a **merge upstream 2.6** entry.
3. mixx-dj-mcp: if CO names or OSC mapping changed, sync `bridge/protocol.py` and re-run `uv run pytest tests`.
4. Push `video` only after local green.

---

## When not to merge yet

- Active gig week (beta instability).
- Large local WIP in the same files upstream touched (finish or stash first).
- No time for video + OSC regression — merging without tests repeats the old "docs say Works" failure mode.

---

## Related docs

- `docs/TODO.md` #34  
- `docs/STEMS-ONNX-PROBE.md` — fork stem compile vs upstream 2.6 stem playback  
- `docs/STATUS.md` — verification vocabulary (Works / Absent / Dead)
