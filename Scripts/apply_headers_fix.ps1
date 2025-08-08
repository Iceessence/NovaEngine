param(
  [string]$RepoRoot = "."
)
$ErrorActionPreference = "Stop"

function Copy-Header($rel) {
  $src = Join-Path $PSScriptRoot ("..\files\" + $rel)
  $dst = Join-Path $RepoRoot $rel
  $dstDir = Split-Path $dst -Parent
  if (-not (Test-Path $src)) { throw "Payload missing: $src" }
  if (-not (Test-Path $dstDir)) { New-Item -ItemType Directory -Force -Path $dstDir | Out-Null }
  if (Test-Path $dst) {
    $backup = "$dst.bak-" + (Get-Date -Format "yyyyMMdd-HHmmss")
    Copy-Item $dst $backup -Force
    Write-Host "Backed up $rel -> $backup"
  } else {
    Write-Host "Creating $rel"
  }
  Copy-Item $src $dst -Force
  Write-Host "Wrote $rel"
}

Copy-Header "src\engine\renderer\vk\VulkanRenderer.h"
Copy-Header "src\engine\editor\Editor.h"
Write-Host "Done. Headers applied."
