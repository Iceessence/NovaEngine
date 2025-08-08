param(
  [Parameter(Mandatory=$true)]
  [string]$RepoRoot
)

function Update-Text($Path, [scriptblock]$Edit) {
  if (-not (Test-Path -LiteralPath $Path)) {
    Write-Host "Missing: $Path" -ForegroundColor Yellow
    return $false
  }
  $t = Get-Content -LiteralPath $Path -Raw
  $new = & $Edit $t
  if ($new -ne $t) {
    Set-Content -LiteralPath $Path -Value $new -NoNewline
    Write-Host "Patched: $Path" -ForegroundColor Green
    return $true
  } else {
    Write-Host "No changes: $Path" -ForegroundColor DarkGray
    return $true
  }
}

$vrh = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.h"
$vrc = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.cpp"
$edh = Join-Path $RepoRoot "src/engine/editor/Editor.h"
$edc = Join-Path $RepoRoot "src/engine/editor/Editor.cpp"

# 1) Fix Log.h includes to the explicit path that exists: src/engine/core/Log.h
$includeFix = {
  param($t)
  $t = $t -replace '(?m)^\s*#\s*include\s*"Log\.h"', '#include "engine/core/Log.h"'
  $t = $t -replace '(?m)^\s*#\s*include\s*<Log\.h>', '#include "engine/core/Log.h"'
  return $t
}
Update-Text $vrc $includeFix | Out-Null
Update-Text $edc $includeFix | Out-Null

# 2) VulkanRenderer.cpp: ensure <algorithm> and VK_CHECK macro exist, and bring std::transform into scope if unqualified is used.
Update-Text $vrc {
  param($t)
  if ($t -notmatch '(?m)^\s*#\s*include\s*<algorithm>') {
    $t = $t -replace '(?m)(^\s*#\s*include\s*<vulkan/vulkan\.h>.*?$)', "`$1`r`n#include <algorithm>"
  }
  if ($t -notmatch '(?m)^\s*#\s*define\s+VK_CHECK\b') {
    $t = $t + @"
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _err = (x); if (_err != VK_SUCCESS) { /* TODO: replace with your logger */ __debugbreak(); } } while(0)
#endif

"@
  }
  # if file uses bare 'transform(' add a using to avoid build break
  if ($t -match '(?<!std::)transform\s*\(' -and $t -notmatch '(?m)^\s*using\s+namespace\s+std;|^\s*using\s+std::transform;') {
    # insert after includes
    $t = $t -replace '(?m)(^\s*#\s*include[^\r\n]*\r?\n(?:\s*#\s*include[^\r\n]*\r?\n)*)', "`$0using std::transform;`r`n"
  }
  return $t
}

# 3) VulkanRenderer.h: add method declarations & members INSIDE the class body
Update-Text $vrh {
  param($t)
  # Ensure we include vulkan header
  if ($t -notmatch '(?m)^\s*#\s*include\s*<vulkan/vulkan\.h>') {
    $t = "#include <vulkan/vulkan.h>`r`n" + $t
  }

  # Find class body
  $m = [regex]::Match($t, 'class\s+VulkanRenderer\b[\s\S]*?\{([\s\S]*?)\n\};', 'Singleline')
  if (-not $m.Success) { return $t } # don't risk corrupting if structure unexpected

  $body = $m.Groups[1].Value

  # public prototypes
  if ($body -notmatch 'InitImGui\s*\(') {
    $body = $body -replace '(?s)(public:\s*)', "`$1`r`n    void InitImGui();`r`n    void BeginFrame();`r`n    void EndFrame(VkCommandBuffer);`r`n    bool IsImGuiReady() const;`r`n"
  }
  # private members
  if ($body -notmatch 'm_imguiReady' -or $body -notmatch 'm_imguiPool') {
    if ($body -match '(?s)private:\s*') {
      $body = $body -replace '(?s)(private:\s*)', "`$1`r`n    bool m_imguiReady = false;`r`n    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;`r`n"
    } else {
      $body = $body + "`r`nprivate:`r`n    bool m_imguiReady = false;`r`n    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;`r`n"
    }
  }

  # Replace body back
  $start = $m.Index
  $len = $m.Length
  $before = $t.Substring(0, $start)
  $after = $t.Substring($start + $len)
  $t = $before + ($m.Value -replace [regex]::Escape($m.Groups[1].Value), [regex]::Escape($body)) + $after

  return $t
}

# 4) Editor.h: declare DrawUI() in class Editor (public)
Update-Text $edh {
  param($t)
  $m = [regex]::Match($t, 'class\s+Editor\b[\s\S]*?\{([\s\S]*?)\n\};', 'Singleline')
  if (-not $m.Success) { return $t }
  $body = $m.Groups[1].Value
  if ($body -notmatch '\bDrawUI\s*\(') {
    $body = $body -replace '(?s)(public:\s*)', "`$1`r`n    void DrawUI();`r`n"
    $t = $t -replace [regex]::Escape($m.Groups[1].Value), [System.Text.RegularExpressions.Regex]::Escape($body)
  }
  return $t
}

# 5) Editor.cpp: fix DockSpaceOverViewport call and add a minimal DrawUI() if missing
Update-Text $edc {
  param($t)
  # Fix DockSpaceOverViewport bad args (ensure GetMainViewport passed)
  if ($t -match 'DockSpaceOverViewport\s*\(' -and $t -notmatch 'GetMainViewport\s*\(') {
    $t = $t -replace 'DockSpaceOverViewport\s*\(\s*([^)]+)\)', 'DockSpaceOverViewport(ImGui::GetMainViewport(), $1)'
  }
  # Add DrawUI definition if missing
  if ($t -notmatch '(?ms)void\s+nova::Editor::DrawUI\s*\(') {
    $t = $t + @"
    
void nova::Editor::DrawUI() {
    ImGuiDockNodeFlags flags = ImGuiDockNodeFlags_None;
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), flags);
}
"@
  }
  return $t
}

Write-Host ""
Write-Host "Done. Now build:" -ForegroundColor Cyan
Write-Host "  cmake --build build --config Debug" -ForegroundColor Cyan
Write-Host "  .\build/bin/Debug/NovaEditor.exe" -ForegroundColor Cyan
