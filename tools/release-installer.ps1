# One-command release for the LACAK Radar web installer.
# Usage:  powershell -ExecutionPolicy Bypass -File tools\release-installer.ps1 -Version 2.0.4
#
# Builds all three display variants, syncs the installer, publishes to Pages.
param(
  [Parameter(Mandatory = $true)]
  [string]$Version,
  [switch]$SkipBuild
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$Version = $Version.TrimStart("v", "V")
if ($Version -notmatch '^\d+\.\d+(\.\d+)?$') {
  throw "Version must look like 2.0.4 (got '$Version')."
}

function Invoke-External {
  param([string]$Exe, [string[]]$Arguments)
  $previous = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  try {
    & $Exe @Arguments 2>&1 | ForEach-Object { Write-Host $_ }
    if ($LASTEXITCODE -ne 0) {
      throw "$Exe $($Arguments -join ' ') failed with exit code $LASTEXITCODE"
    }
  } finally {
    $ErrorActionPreference = $previous
  }
}

Push-Location $root
try {
  $sketchPath = Join-Path $root "firmware\ESP32_Radar\ESP32_Radar.ino"
  $sketch = Get-Content $sketchPath -Raw
  $updated = [regex]::Replace(
    $sketch,
    '(#define\s+FW_VERSION\s+")[^"]*(")',
    '${1}' + $Version + '${2}'
  )
  if ($updated -ne $sketch) {
    Set-Content -Path $sketchPath -Value $updated -NoNewline
    Write-Host "Firmware version set to $Version"
  } else {
    Write-Host "Firmware version already $Version"
  }

  if (-not $SkipBuild) {
    foreach ($env in @("esp32dev", "esp32dev-177", "esp32dev-nextion")) {
      Write-Host "`nBuilding $env..."
      Invoke-External "pio" @("run", "-e", $env)
    }
  } else {
    Write-Host "`nSkipping builds, using existing .pio output"
  }

  $sync = Join-Path $root "tools\sync-installer-firmware.ps1"
  $publish = Join-Path $root "tools\publish-installer.ps1"

  Write-Host "`nUpdating installer files..."
  Invoke-External "powershell" @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $sync, "-Version", $Version
  )

  Write-Host "`nPublishing to GitHub Pages..."
  Invoke-External "powershell" @(
    "-NoProfile", "-ExecutionPolicy", "Bypass", "-File", $publish
  )

  Write-Host ""
  Write-Host "Release v$Version published" -ForegroundColor Green
  Write-Host "https://bytebleed2025-star.github.io/ESP32-Radar-installer/"
  Write-Host ""
  Write-Host "Commit the source + installer files when convenient:"
  Write-Host "  git add -A"
  Write-Host "  git commit -m `"Release v$Version`""
  Write-Host "  git push"
} finally {
  Pop-Location
}
