@echo off
REM Mixxxxx fleet launcher — OSC defaults for mixx-dj-mcp (11119 in / 11118 out)
setlocal
set "DIR=%~dp0"
start "" "%DIR%mixxx.exe" --osc-port-in=11119 --osc-port-out=11118 --osc-host-out=127.0.0.1 %*
