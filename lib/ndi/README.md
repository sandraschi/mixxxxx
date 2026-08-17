# NDI SDK headers (MIT)

These files are the **header-only** portion of the [NDI SDK](https://ndi.video/for-developers/ndi-sdk/)
that Vizrt grants to open-source projects under the **MIT license** (see the copyright block at the
top of each `Processing.NDI.*.h` file).

They were copied from [DistroAV/DistroAV `lib/ndi/`](https://github.com/DistroAV/DistroAV/tree/master/lib/ndi)
(formerly obs-ndi), which ships the same MIT header bundle. Version: see `include/Version.txt`.

**Not included (and must never be committed):**

- NDI SDK source
- Import libraries (`.lib`)
- Runtime binaries (`Processing.NDI.Lib.x64.dll`, `libndi.so`, etc.)

Mixxxxx loads the proprietary NDI runtime **dynamically at runtime** only. See `docs/NDI-LICENSING.md`.
