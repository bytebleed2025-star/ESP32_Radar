# Publishes docs/install to the public GitHub Pages repo.
# Usage:  powershell -ExecutionPolicy Bypass -File tools\publish-installer.ps1
param(
  [string]$Repo = "https://github.com/bytebleed2025-star/ESP32-Radar-installer.git"
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot
$source = Join-Path $root "docs"
$tmp = Join-Path $env:TEMP "radar-installer-publish"

function Invoke-Git {
  param([string[]]$GitArgs)
  $previous = $ErrorActionPreference
  $ErrorActionPreference = "Continue"
  try {
    $output = & git @GitArgs 2>&1
    $code = $LASTEXITCODE
    $output | ForEach-Object { Write-Host $_ }
    if ($code -ne 0) { throw "git $($GitArgs -join ' ') failed with exit code $code" }
  } finally {
    $ErrorActionPreference = $previous
  }
}

if (Test-Path (Join-Path $tmp ".git")) {
  Invoke-Git @("-C", $tmp, "fetch", "origin")
  Invoke-Git @("-C", $tmp, "reset", "--hard", "origin/main")
} else {
  if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
  Invoke-Git @("clone", $Repo, $tmp)
}

Get-ChildItem $tmp -Force |
  Where-Object { $_.Name -ne ".git" } |
  Remove-Item -Recurse -Force

Copy-Item (Join-Path $source "install\*") $tmp -Recurse -Force
Copy-Item (Join-Path $source ".nojekyll") (Join-Path $tmp ".nojekyll") -Force

Invoke-Git @("-C", $tmp, "-c", "core.autocrlf=false", "add", "-A")

$previous = $ErrorActionPreference
$ErrorActionPreference = "Continue"
$changes = & git -C $tmp status --porcelain 2>$null
$statusCode = $LASTEXITCODE
$ErrorActionPreference = $previous
if ($statusCode -ne 0) { throw "git status failed with exit code $statusCode" }
if (-not $changes) {
  Write-Host "Installer is already up to date."
  exit 0
}

Invoke-Git @("-C", $tmp, "commit", "-m", "Update LACAK Radar installer")
Invoke-Git @("-C", $tmp, "push", "origin", "main")

Write-Host "Published: https://bytebleed2025-star.github.io/ESP32-Radar-installer/"
