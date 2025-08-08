param(
  [Parameter(Mandatory=$true)][string]$RepoRoot
)
$ErrorActionPreference = "Stop"

$targets = @(
  "src/engine/editor/Editor.cpp",
  "src/engine/renderer/vk/VulkanRenderer.cpp"
)

foreach ($rel in $targets) {
  $p = Join-Path $RepoRoot $rel
  if (-not (Test-Path $p)) { Write-Host "Skip: $rel (not found)"; continue }
  $txt = Get-Content $p -Raw
  $orig = $txt
  # Prefer explicit engine include to avoid search path ambiguity
  $txt = $txt -replace '(?m)^\s*#\s*include\s*["<]Log\.h[">]', '#include "core/Log.h"'
  if ($txt -ne $orig) {
    Set-Content -Path $p -Value $txt -NoNewline
    Write-Host "Patched include in $rel"
  } else {
    Write-Host "No include change needed in $rel"
  }
}
Write-Host "Include patch completed."
