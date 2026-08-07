# ONNX stems build probe (TODO 31)

`StemSeparator` lives in `src/stems/stem_separator.cpp` but is **only compiled when** `-DONNX_RUNTIME=ON` on Windows.

## What the probe does

`scripts/probe-onnx-stems.ps1`:

1. Reconfigures `build/` with `ONNX_RUNTIME=ON` (downloads ONNX Runtime 1.20.1 via CMake FetchContent)
2. Builds `mixxx-lib` (and copies `onnxruntime.dll`)
3. Reports whether `__ONNX_RUNTIME__` objects link

Does **not** ship a Demucs model — separation still needs a model file on disk after load.

## Run (x64 Native Tools Command Prompt for VS 2022)

```powershell
tools\windows_release_buildenv.bat
.\scripts\probe-onnx-stems.ps1
```

Optional full binary:

```powershell
.\scripts\probe-onnx-stems.ps1 -FullBuild
```

## Success criteria

| Check | Meaning |
|-------|---------|
| CMake configures with `ONNX_RUNTIME=ON` | FetchContent + include paths OK |
| `ninja mixxx-lib` succeeds | `stem_separator.cpp` compiles |
| `onnxruntime.dll` beside `mixxx-lib` | Runtime bundled for dev |

## After probe passes

1. Obtain Demucs ONNX model (see `mcp-central-docs/projects/mixxxxx/STEMS_AND_CROSS_CONNECT.md`)
2. Wire `stem_separate` CO / MCP tool
3. Optional: mixx-dj-mcp stem provider calling mixxxxx OSC

## Current production build

Default fleet build keeps `ONNX_RUNTIME=OFF` — smaller installer, no ~200MB model fetch.

Status tracked in `docs/STATUS.md` (Stem separation: Absent from binary).

## Verified on goliath (2026-07-27)

| Check | Result |
|-------|--------|
| `just probe-onnx-stems` | Pass |
| `stem_separator.cpp.obj` | Present under `build/CMakeFiles/mixxx-lib.dir/` |
| `onnxruntime.dll` | `build/onnxruntime.dll` |

Demucs model + runtime CO wiring still open.
