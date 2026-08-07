# --- Mixxxxx  local recipes  Windows-first ---
set windows-shell := ["powershell.exe", "-NoProfile", "-Command"]

default:
    @just --list

help:
    @just --list

build:
    @powershell.exe -NoProfile -Command "Set-Location '{{justfile_directory()}}'; ninja -C build"

brand-assets:
    @powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{{justfile_directory()}}/packaging/mixxxxx/generate-branding.ps1"

build-installer Generator="NSIS":
    @powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{{justfile_directory()}}/scripts/build-installer.ps1" -Generator {{Generator}}

build-installer-msi:
    @just build-installer "WIX"

probe-onnx-stems:
    @powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{{justfile_directory()}}/scripts/probe-onnx-stems.ps1"

probe-onnx-stems-full:
    @powershell.exe -NoProfile -ExecutionPolicy Bypass -File "{{justfile_directory()}}/scripts/probe-onnx-stems.ps1" -FullBuild

# Bootstrap: install dev deps + pre-commit hook
bootstrap:
    uv sync --group dev
    uv run pre-commit install
    Write-Host "Pre-commit hooks installed." -ForegroundColor Green