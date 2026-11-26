# Copies the three Radar variant builds into the web installer.
# Usage:  powershell -ExecutionPolicy Bypass -File tools\sync-installer-firmware.ps1 [-Version 2.0.3]
param(
  [string]$Version = "2.0.3"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$destRoot = Join-Path $root "docs\install"
$fwRoot = Join-Path $destRoot "firmware"

$bootApp0 = Get-ChildItem (Join-Path $env:USERPROFILE ".platformio\packages") `
  -Recurse -Filter "boot_app0.bin" -ErrorAction SilentlyContinue |
  Select-Object -First 1
if (-not $bootApp0) {
  throw "boot_app0.bin not found under .platformio\packages"
}

$variants = @(
  @{ env = "esp32dev";         dir = "180";     manifest = "manifest-180.json" },
  @{ env = "esp32dev-177";     dir = "177";     manifest = "manifest-177.json" },
  @{ env = "esp32dev-nextion"; dir = "nextion"; manifest = "manifest-nextion.json" }
)

foreach ($v in $variants) {
  $build = Join-Path $root ".pio\build\$($v.env)"
  $firmware = Join-Path $build "firmware.bin"
  if (-not (Test-Path $firmware)) {
    throw "Missing $firmware - build $($v.env) first."
  }

  $out = Join-Path $fwRoot $v.dir
  New-Item -ItemType Directory -Force -Path $out | Out-Null
  Copy-Item (Join-Path $build "bootloader.bin") $out -Force
  Copy-Item (Join-Path $build "partitions.bin") $out -Force
  Copy-Item $bootApp0.FullName (Join-Path $out "boot_app0.bin") -Force
  Copy-Item $firmware (Join-Path $out "firmware.bin") -Force

  $manifestPath = Join-Path $destRoot $v.manifest
  $manifest = Get-Content $manifestPath -Raw | ConvertFrom-Json
  $manifest.version = $Version
  $manifest | ConvertTo-Json -Depth 8 | Set-Content $manifestPath -Encoding utf8

  $size = [math]::Round((Get-Item $firmware).Length / 1MB, 2)
  Write-Host "Updated $($v.dir): $($v.env) v$Version ($size MB)"
}

$apkSrc = Join-Path $root "bin\v2.0.3\DetektorAir-Bluetooth-v2.0.3g.apk"
if (Test-Path $apkSrc) {
  $apkDir = Join-Path $destRoot "apk"
  New-Item -ItemType Directory -Force -Path $apkDir | Out-Null
  Copy-Item $apkSrc (Join-Path $apkDir "DetektorAir-v2.0.3g.apk") -Force
  Write-Host "Copied APK v2.0.3g"
} else {
  Write-Host "WARNING: APK source not found: $apkSrc"
}
