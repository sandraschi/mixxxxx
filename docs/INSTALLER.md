# Mixxxxx Windows installer

Branded packaging for the **video** fork. Fleet standard is **NSIS** (single `.exe` setup); **WIX/MSI** remains available for enterprise.

## Prerequisites

1. Completed **RelWithDebInfo** or **Release** build (`build/mixxx.exe`)
2. **NSIS 3.x** installed and on `PATH` (`makensis /VERSION`)
3. CMake configure with **`-DMIXXXXX_BRANDING=ON`** (default in this repo)

Close running `mixxx.exe` before packaging — the NSIS hook also attempts `taskkill`.

## Quick path

```powershell
tools\windows_release_buildenv.bat
cd build
cmake -DCMAKE_TOOLCHAIN_FILE="..\buildenv\mixxx-deps-2.5-x64-windows-release-40c29ff\scripts\buildsystems\vcpkg.cmake" `
      -DVCPKG_TARGET_TRIPLET=x64-windows-release -G Ninja `
      -DMIXXXXX_BRANDING=ON ..
ninja
cd ..
just build-installer
```

Output (typical): `build/mixxxxx-2.5.6-video.N-win64.exe`

## Generators

| Generator | Artifact | Use |
|-----------|----------|-----|
| **NSIS** (default) | `mixxxxx-*-win64.exe` | Fleet / GitHub Releases |
| **WIX** | `mixxxxx-*-win64.msi` | Enterprise / Group Policy |

```powershell
.\scripts\build-installer.ps1 -Generator NSIS
.\scripts\build-installer.ps1 -Generator WIX
```

## Branding (`MIXXXXX_BRANDING`)

When `ON` (default):

- Product name **Mixxxxx**, install dir `%ProgramFiles%\Mixxxxx`
- Start menu shortcut **Mixxxxx** → `mixxx.exe`
- Package description in `packaging/mixxxxx/CPackPackageDescription.txt`
- NSIS pre-install: stop `mixxx.exe`
- Filename prefix `mixxxxx-` (not `mixxx-`)

Set `-DMIXXXXX_BRANDING=OFF` to restore upstream **Mixxx** WIX/MSI naming.

## Branding assets

Before packaging, generate icons (Inkscape if installed, else fallback render):

```powershell
.\packaging\mixxxxx\generate-branding.ps1
```

Produces `packaging/mixxxxx/ic_mixxxxx.ico`, `mixxxxx_install_logo.bmp`, `mixxxxx_install_banner.bmp`.

## Post-install shortcuts

The installer creates:

| Shortcut | Target |
|----------|--------|
| **Mixxxxx (OSC fleet)** | `mixxxxx-osc.cmd` → `mixxx.exe --osc-port-in=11119 --osc-port-out=11118 --osc-host-out=127.0.0.1` |
| **Mixxxxx** | Plain `mixxx.exe` |
| Desktop | OSC fleet launcher |

## Not bundled

| Component | Notes |
|-----------|--------|
| **NDI SDK/runtime** | User installs NDI Tools; see `docs/NDI-LICENSING.md` |
| **ONNX / stems model** | `ONNX_RUNTIME=OFF` in default build |
| **mixx-dj-mcp** | Separate Tauri installer in `mixx-dj-mcp` |

## Install flow (users)

1. Optional: uninstall upstream Mixxx (keep `%LOCALAPPDATA%\Mixxx\mixxxdb.sqlite`)
2. Run `mixxxxx-*-win64-setup.exe`
3. Launch **Mixxxxx** — OSC defaults **11119 in / 11118 out** (mixxxxx CLI flags or Preferences)
4. Install **mixx-dj-mcp** operator for MCP/webapp control

## CI / releases

Per fleet policy: build installers **locally**, upload to GitHub Releases manually. Do not rely on private-repo GHA for release artifacts.

## Files

```
packaging/mixxxxx/
  CPackPackageDescription.txt
  nsis-extra.nsh          # CPack NSIS hooks
scripts/build-installer.ps1
justfile                  # just build-installer
```
