# Generate Mixxxxx installer branding (ico + bmp) for CPack NSIS.
# Prefers Inkscape for SVG fidelity; falls back to System.Drawing.

param(
    [string]$SvgPath = "",
    [string]$OutDir = ""
)

$ErrorActionPreference = "Stop"
$scriptDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$repoRoot = Split-Path -Parent (Split-Path -Parent $scriptDir)
if (-not $SvgPath) { $SvgPath = Join-Path $repoRoot "res\images\mixxxxx-banner.svg" }
if (-not $OutDir) { $OutDir = $scriptDir }

$iconOut = Join-Path $OutDir "ic_mixxxxx.ico"
$logoOut = Join-Path $OutDir "mixxxxx_install_logo.bmp"
$bannerOut = Join-Path $OutDir "mixxxxx_install_banner.bmp"

function Invoke-InkscapeExport {
    param([string]$InputSvg, [string]$OutputPng, [int]$Width, [int]$Height = 0)
    $inkscape = @(
        "${env:ProgramFiles}\Inkscape\bin\inkscape.exe",
        "${env:ProgramFiles(x86)}\Inkscape\bin\inkscape.exe"
    ) | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $inkscape) { return $false }
    $args = @($InputSvg, "--export-type=png", "--export-filename=$OutputPng", "-w", $Width)
    if ($Height -gt 0) { $args += @("-h", $Height) }
    & $inkscape @args | Out-Null
    return (Test-Path $OutputPng)
}

function Save-BitmapAsIco {
    param([System.Drawing.Bitmap]$Bitmap, [string]$Path)
    $hIcon = $Bitmap.GetHicon()
    $icon = [System.Drawing.Icon]::FromHandle($hIcon)
    $fs = [System.IO.File]::Create($Path)
    try { $icon.Save($fs) } finally { $fs.Close() }
}

function New-FallbackBitmap {
    param([int]$Width, [int]$Height, [string]$Title, [string]$Subtitle = "")
    Add-Type -AssemblyName System.Drawing
    $bmp = New-Object System.Drawing.Bitmap $Width, $Height
    $g = [System.Drawing.Graphics]::FromImage($bmp)
    $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $g.Clear([System.Drawing.Color]::FromArgb(255, 13, 9, 6))
    $titleSize = [float]([Math]::Max(14, $Width / 14))
    $subSize = [float]([Math]::Max(8, $Width / 28))
    $titleFont = [System.Drawing.Font]::new("Segoe UI", $titleSize, [System.Drawing.FontStyle]::Bold, [System.Drawing.GraphicsUnit]::Pixel)
    $subFont = [System.Drawing.Font]::new("Segoe UI", $subSize, [System.Drawing.FontStyle]::Regular, [System.Drawing.GraphicsUnit]::Pixel)
    $amber = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 255, 140, 32))
    $muted = New-Object System.Drawing.SolidBrush ([System.Drawing.Color]::FromArgb(255, 200, 184, 150))
    $y = [Math]::Max(8, ($Height / 2) - 24)
    $g.DrawString($Title, $titleFont, $amber, 12, $y)
    if ($Subtitle) {
        $g.DrawString($Subtitle, $subFont, $muted, 12, ($y + 36))
    }
    $g.Dispose()
    return $bmp
}

Add-Type -AssemblyName System.Drawing

$tmpPng = Join-Path $env:TEMP "mixxxxx-brand-256.png"
$usedInkscape = $false
if (Test-Path $SvgPath) {
    $usedInkscape = Invoke-InkscapeExport -InputSvg $SvgPath -OutputPng $tmpPng -Width 256
}

if ($usedInkscape -and (Test-Path $tmpPng)) {
    $src = [System.Drawing.Image]::FromFile($tmpPng)
    $square = New-Object System.Drawing.Bitmap 256, 256
    $g = [System.Drawing.Graphics]::FromImage($square)
    $g.DrawImage($src, 0, 0, 256, 256)
    $g.Dispose()
    $src.Dispose()
    Save-BitmapAsIco -Bitmap $square -Path $iconOut
    $square.Dispose()
    Write-Host "Icon from Inkscape: $iconOut"
}
else {
    $bmp = New-FallbackBitmap -Width 256 -Height 256 -Title "Mixxxxx" -Subtitle "video · OSC"
    Save-BitmapAsIco -Bitmap $bmp -Path $iconOut
    $bmp.Dispose()
    Write-Host "Icon from fallback render: $iconOut"
}

# NSIS welcome icon (~55x55) and side banner (164x314)
$logo = New-FallbackBitmap -Width 55 -Height 55 -Title "Mx"
$logo.Save($logoOut, [System.Drawing.Imaging.ImageFormat]::Bmp)
$logo.Dispose()

$banner = New-FallbackBitmap -Width 164 -Height 314 -Title "Mixxxxx" -Subtitle "OSC 11119/11118 · mixx-dj-mcp"
$banner.Save($bannerOut, [System.Drawing.Imaging.ImageFormat]::Bmp)
$banner.Dispose()

Write-Host "Logo: $logoOut"
Write-Host "Banner: $bannerOut"
if ($usedInkscape) { Remove-Item $tmpPng -ErrorAction SilentlyContinue }
