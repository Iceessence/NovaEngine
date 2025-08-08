param(
  [Parameter(Mandatory=$true)][string]$RepoRoot
)
$ErrorActionPreference = "Stop"
$ed = Join-Path $RepoRoot "src/engine/editor/Editor.cpp"
if (-not (Test-Path $ed)) { throw "File not found: $ed" }
$txt = Get-Content $ed -Raw
$orig = $txt

# Insert a small static helper if not present
if ($txt -notmatch 'static\s+bool\s+__NovaImGuiReady') {
  $helper = @"
static bool __NovaImGuiReady()
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) return false;
    ImGuiIO& io = ImGui::GetIO();
    if (!(io.BackendRendererUserData && io.BackendPlatformUserData)) return false;
    if (!(io.Fonts && io.Fonts->IsBuilt())) return false;
    return true;
}
"@

  # After first #include <imgui.h> or "imgui.h"
  $txt = $txt -replace '(?m)(#\s*include\s*[<"]imgui\.h[>].*?\n)', "`$1`n$helper`n"
  $inserted = $true
}

# Replace occurrences of if (!ui_frame_begun) return; with new guard
$txt = $txt -replace '(?m)if\s*\(\s*!ui_frame_begun\s*\)\s*return\s*;', 'if (!__NovaImGuiReady()) return;'

if ($txt -ne $orig) {
  Set-Content -Path $ed -Value $txt -NoNewline
  Write-Host "Editor UI guard applied."
} else {
  Write-Host "No guard changes were necessary."
}
