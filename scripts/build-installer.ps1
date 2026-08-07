# Build Mixxxxx-branded Windows installer (CPack NSIS by default).
# Prerequisites: Release/RelWithDebInfo build in .\build, NSIS on PATH (CPack finds makensis).
#
# Usage:
#   .\scripts\build-installer.ps1
#   .\scripts\build-installer.ps1 -BuildDir build -Generator NSIS
#   .\scripts\build-installer.ps1 -Generator WIX   # MSI, Mixxx-style upstream

param(
    [string]$BuildDir = "build",
    [ValidateSet("NSIS", "WIX")]
    [string]$Generator = "NSIS"
)

$ErrorActionPreference = "Stop"

$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent $scriptDir
$buildPath = Join-Path $repoRoot $BuildDir
$exePath = Join-Path $buildPath "mixxx.exe"

if (-not (Test-Path $exePath)) {
    Write-Error @"
mixxx.exe not found at $exePath
Build first:
  tools\windows_release_buildenv.bat
  cd build
  cmake -DCMAKE_TOOLCHAIN_FILE=..\buildenv\mixxx-deps-2.5-x64-windows-release-40c29ff\scripts\buildsystems\vcpkg.cmake `
        -DVCPKG_TARGET_TRIPLET=x64-windows-release -G Ninja `
        -DMIXXXXX_BRANDING=ON ..
  ninja
"@
}

$nsisCandidates = @(
    "${env:ProgramFiles(x86)}\NSIS",
    "$env:ProgramFiles\NSIS"
) | Where-Object { Test-Path (Join-Path $_ "makensis.exe") }
if ($nsisCandidates.Count -gt 0 -and -not (Get-Command makensis -ErrorAction SilentlyContinue)) {
    $env:PATH = "$($nsisCandidates[0]);$env:PATH"
    Write-Host "Prepended NSIS to PATH: $($nsisCandidates[0])"
}

$running = Get-Process -Name mixxx -ErrorAction SilentlyContinue
if ($running) {
    Write-Host "Stopping mixxx.exe (PID $($running.Id -join ', '))..."
    $running | Stop-Process -Force
    Start-Sleep -Seconds 2
}

Push-Location $buildPath
try {
    Write-Host "Running cpack -G $Generator in $buildPath"
    & cpack -G $Generator -V
    if ($LASTEXITCODE -ne 0) {
        throw "cpack failed with exit code $LASTEXITCODE"
    }

    $patterns = @("mixxxxx-*.exe", "mixxxxx-*.msi", "mixxx-*.msi")
    $artifacts = @()
    foreach ($pat in $patterns) {
        $artifacts += Get-ChildItem -Path $buildPath -Filter $pat -ErrorAction SilentlyContinue
    }
    if ($artifacts.Count -eq 0) {
        Write-Warning "cpack finished but no mixxxxx/mixxx installer found in $buildPath"
    }
    else {
        Write-Host "`nInstaller artifact(s):"
        foreach ($a in $artifacts) {
            Write-Host "  $($a.FullName)  ($([math]::Round($a.Length / 1MB, 1)) MB)"
        }
    }
}
finally {
    Pop-Location
}
