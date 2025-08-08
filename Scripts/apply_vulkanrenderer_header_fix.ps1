param(
  [string]$RepoRoot = "."
)
$ErrorActionPreference = "Stop"
$repo = Resolve-Path $RepoRoot
$target = Join-Path $repo "src\engine\renderer\vk\VulkanRenderer.h"
$src = Join-Path $PSScriptRoot "..\files\src\engine\renderer\vk\VulkanRenderer.h"

if (-not (Test-Path $target)) {
  New-Item -ItemType Directory -Path (Split-Path $target) -Force | Out-Null
}

if (Test-Path $target) {
  $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
  Copy-Item $target "$target.bak-$stamp"
}
Copy-Item $src $target -Force
Write-Host "Applied VulkanRenderer.h fix -> $target"
