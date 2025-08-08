param(
  [Parameter(Mandatory=$false)][string]$RepoRoot = "."
)
$ErrorActionPreference = "Stop"
$repo = (Resolve-Path $RepoRoot).Path
$editor = Join-Path $repo "src\engine\editor\Editor.cpp"
if (!(Test-Path $editor)) {
  Write-Error "Editor.cpp not found at $editor"
}

$content = Get-Content $editor -Raw

$shim = @"
/*** NOVA UI-FRAME SHIM (auto-inserted) ***/
namespace nova { static bool g_ui_frame_begun = false; }
using nova::g_ui_frame_begun;
#ifndef ui_frame_begun
#define ui_frame_begun g_ui_frame_begun
#endif
/*** END SHIM ***/

"@

if ($content -match "NOVA UI-FRAME SHIM") {
  Write-Host "Shim already present. No changes made."
} else {
  Copy-Item $editor "$editor.bak" -Force
  # Insert shim after the last include block to keep it early in the file
  $pattern = "(?s)(#include[^\r\n]*[\r\n])+"
  $new = [System.Text.RegularExpressions.Regex]::Replace($content, $pattern, {
    param($m)
    return $m.Value + "`r`n" + $shim
  }, 1)
  if ($new -eq $content) {
    # Fallback: prepend
    $new = $shim + $content
  }
  Set-Content $editor $new -NoNewline
  Write-Host "Shim inserted into $editor"
}
