# NDI licensing and GPL compliance

Mixxxxx is **GPL v2-or-later**. The NDI SDK runtime is **proprietary**. Static or dynamic
**linking** NDI into our binary would create a license conflict and prevent distributing
Mixxxxx under GPL.

This document describes how NDI output is integrated so the fork stays **provably clean**,
following the approach NDI documents for open-source projects: **MIT headers in the repo**,
**dynamic load at runtime**, **no bundled runtime**.

---

## Audit (pre-fix violations)

Audit date: 2026-07-27. Findings before this change:

| # | Requirement | Violation (file:line) |
|---|-------------|------------------------|
| 1 | Headers only in repo (MIT) | **No** NDI headers in tree; `CMakeLists.txt:3703–3704` used `find_path(NDI_INCLUDE_DIR …)` against a system SDK install |
| 2 | Dynamic load only | `ndioutput.cpp:12` `#include <Processing.NDI.Lib.h>`; direct calls to `NDIlib_initialize`, `NDIlib_send_create`, `NDIlib_send_send_video_v2` (`ndioutput.cpp:24–33`, `119`, `188`); `CMakeLists.txt:3705–3712` `find_library` + `target_link_libraries` |
| 3 | Do not bundle/install runtime | `CMakeLists.txt:3714–3718` `install(FILES Processing.NDI.Lib.x64.dll …)` |
| 4 | NDI visual identification in UI | `dlgstartupbanner.cpp:48` generic “NDI” text only; no ® mark or runtime guidance |
| 5 | No NDI HX | **No violations found** — no HX / Advanced SDK symbols in tree (grep 2026-07-27) |
| 6 | Send off realtime/audio thread | Send ran on **QTimer** (GUI thread), not audio callback — **partial OK** for audio path; but `NDIlib_send_send_video_v2` on timer thread with **no sender thread, no bounded queue, no drop policy** (`ndioutput.cpp:153–188`) |

**Not verified in CI:** end-to-end feed in OBS / NDI Studio Monitor with a live runtime installed.

---

## Current design (post-fix)

### 1. Headers only (MIT)

Vendored under `lib/ndi/include/` — see `lib/ndi/README.md` and `lib/ndi/include/Version.txt`.
Each `Processing.NDI.*.h` file carries Vizrt’s MIT license header. **No** SDK source, `.lib`,
or `.dll` is committed.

### 2. Dynamic load at runtime

- `src/video/ndi_runtime.{h,cpp}` loads `Processing.NDI.Lib.x64.dll` (or platform equivalent)
  via `LoadLibrary` / `dlopen`, resolves `NDIlib_v5_load` (fallback `NDIlib_v6_load`), and
  calls NDI only through the returned function table.
- **No** `target_link_libraries` against NDI.
- **No** compile-time dependency on an SDK install path.

### 3. Runtime not bundled

Users install the [NDI redistributable](https://ndi.link/NDIRedistV5) separately.

Discovery order:

1. Environment variable `NDI_RUNTIME_DIR_V5`
2. Environment variable `NDI_RUNTIME_DIR_V6` (newer runtimes; headers are v6.3-compatible)
3. Environment variable named by `NDILIB_REDIST_FOLDER` in the MIT headers (`NDI_RUNTIME_DIR_V6`)

If loading fails, NDI output is disabled, a clear log line is written, and enabling NDI shows a
dialog with a link to the official redist download. Mixxxxx **builds and runs** without the runtime.

### 4. Visual identification

UI that mentions NDI output uses **NDI®** (registered trademark) and points users to the official
runtime download. See startup banner and the runtime-missing dialog in `NdiOutput`.

### 5. No NDI HX

Only standard **send** APIs (`send_create`, `send_send_video_v2`) are used. NDI HX and the
Advanced SDK are **not** referenced. HX would require a commercial license incompatible with GPL.

### 6. Sender thread

- **Capture** (blend + letterbox) runs on the GUI `QTimer` thread.
- **Send** runs on `NdiSenderWorker` (`QThread`) with a **bounded queue (depth 2)**; oldest frames
  are dropped when the sender falls behind — never blocking the timer or audio path.

---

## Files from the NDI SDK (MIT headers in repo)

| File | Purpose |
|------|---------|
| `Processing.NDI.compat.h` | Compatibility macros |
| `Processing.NDI.structs.h` | Shared structs (e.g. video frames) |
| `Processing.NDI.Find.h` | Discovery API declarations |
| `Processing.NDI.Recv.h` | Receive API declarations |
| `Processing.NDI.Recv.ex.h` | Receive extensions |
| `Processing.NDI.RecvAdvertiser.h` | Receive advertiser |
| `Processing.NDI.RecvListener.h` | Receive listener |
| `Processing.NDI.Send.h` | **Send API** (used by Mixxxxx) |
| `Processing.NDI.SendAdvertiser.h` | Send advertiser |
| `Processing.NDI.SendListener.h` | Send listener |
| `Processing.NDI.Routing.h` | Routing API declarations |
| `Processing.NDI.utilities.h` | Utility declarations |
| `Processing.NDI.deprecated.h` | Deprecated aliases |
| `Processing.NDI.FrameSync.h` | Frame sync declarations |
| `Processing.NDI.DynamicLoad.h` | **Dynamic load** entry points and v5/v6 function tables |
| `Processing.NDI.Lib.h` | Umbrella header (included by reference projects) |
| `Processing.NDI.Lib.cplusplus.h` | C++ helpers (not required at link time) |
| `Version.txt` | Header bundle version string |
| `mixxxxx_ndi_headers.h` | Mixxxxx wrapper (`PROCESSINGNDILIB_STATIC` + umbrella include) |

**License:** MIT per-file notice in each header (Copyright Vizrt NDI AB). The **runtime DLL**
remains under NDI’s proprietary redistributable terms; users accept those when installing NDI Tools
/ redist — Mixxxxx does not ship it.

---

## Build option

```cmake
option(NDI "NDI network video output" ON)  # requires FFMPEG=ON
```

- `NDI=ON` (default): compiles `ndi_runtime.cpp`, defines `__MIXXXXX_NDI__`, still **no link** to NDI.
- `NDI=OFF`: `NdiOutput` stub; no NDI headers compiled into send path.

---

## Why this keeps Mixxxxx distributable under GPL v2+

GPL applies to **combined works** where proprietary object code is linked into the same binary.
Here:

- Git contains only **MIT-licensed headers** (permitted in a GPL project).
- The GPL binary **does not embed** NDI object code; it optionally `LoadLibrary`s a **user-installed**
  runtime at runtime (same pattern as DistroAV/obs-ndi and NDI’s documented OSS approach).
- We **do not** redistribute the proprietary runtime.

Users who enable NDI must install Vizrt’s redist under its terms, separately from Mixxxxx.

---

## References

- [NDI SDK — dynamic loading](https://docs.ndi.video/all/developing-with-ndi/sdk/dynamic-loading-of-ndi-libraries)
- [NDI redistributable (V5)](https://ndi.link/NDIRedistV5)
- [NDI redistributable (V6)](https://ndi.link/NDIRedistV6)
- Mixxxxx implementation: `src/video/ndi_runtime.{h,cpp}`, `src/video/ndioutput.{h,cpp}`
