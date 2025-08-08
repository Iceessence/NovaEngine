param([string]$RepoRoot = ".")
function Edit-File($path, [ScriptBlock]$fn) {
  $t = Get-Content -Raw -Path $path -ErrorAction Stop
  $n = & $fn $t
  if ($n -ne $t) { Set-Content -Path $path -Value $n -Encoding UTF8; Write-Host "Patched: $path"; $true } else { Write-Host "No changes: $path"; $false }
}

# 1) VK_CHECK macro in VulkanHelpers.h (add if missing)
$helpers = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanHelpers.h'
if (Test-Path $helpers) {
  Edit-File $helpers {
    param($s)
    if ($s -notmatch 'VK_CHECK') {
      if ($s -notmatch '<cstdio>') { $s = "#include <cstdio>`n#include <cstdlib>`n" + $s }
      $s += @"
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _e=(x); if(_e){ fprintf(stderr,""VK_CHECK failed %d at %s:%d\n"", (int)_e, __FILE__, __LINE__); abort(); } } while(0)
#endif
"@
    }
    return $s
  } | Out-Null
}

# 2) VulkanRenderer.cpp: make ImGui init 1-arg and remove duplicate IsImGuiReady
$vr_cpp = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.cpp'
if (Test-Path $vr_cpp) {
  Edit-File $vr_cpp {
    param($s)
    # Two-arg -> one-arg; stash 2nd arg into RenderPass
    $s = [regex]::Replace($s, 'ImGui_ImplVulkan_Init\s*\(\s*&?(\w+)\s*,\s*([^)]+?)\s*\)\s*;', ' $1.RenderPass = $2;`n    ImGui_ImplVulkan_Init(&$1);', 'Singleline')
    # Prefer explicit UseDynamicRendering=false when we use RenderPass
    if ($s -match 'ImGui_ImplVulkan_InitInfo\s+(\w+)\s*=\s*\{') {
      $v = $Matches[1]
      if ($s -notmatch [regex]::Escape("$v.UseDynamicRendering")) {
        $s = $s -replace "ImGui_ImplVulkan_InitInfo\s+$v\s*=\s*\{", "ImGui_ImplVulkan_InitInfo $v = {`n        .UseDynamicRendering = false,"
      }
    }
    # Remove any cpp-side IsImGuiReady (keep header inline getter)
    $s = [regex]::Replace($s, 'bool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}', '', 'Singleline')
    return $s
  } | Out-Null
}

# 3) VulkanRenderer.h: ensure a single inline IsImGuiReady and InitImGui declared
$vr_h = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.h'
if (Test-Path $vr_h) {
  Edit-File $vr_h {
    param($s)
    $s = [regex]::Replace($s, 'bool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{[^}]*\}', 'bool IsImGuiReady() const { return m_imguiReady; }')
    if ($s -notmatch 'void\s+InitImGui\s*\(\s*GLFWwindow\s*\*\s*\w*\s*\)') {
      $s = $s -replace 'class\s+VulkanRenderer\s*\{', "class VulkanRenderer{`npublic:`n    void InitImGui(GLFWwindow* window);"
    }
    return $s
  } | Out-Null
}

# 4) Editor.cpp: fix DockSpaceOverViewport arg order + remove 2nd DrawUI if present
$ed_cpp = Join-Path $RepoRoot 'src\engine\editor\Editor.cpp'
if (Test-Path $ed_cpp) {
  function Remove-SecondDrawUI([string]$src) {
    $rx = [regex]'void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{'
    $m = $rx.Matches($src)
    if ($m.Count -gt 1) {
      $start = $m[1].Index
      $i=$start; $depth=0; $len=$src.Length
      while ($i -lt $len) {
        $ch = $src[$i]
        if ($ch -eq '{') { $depth++ }
        elseif ($ch -eq '}') { $depth--; if ($depth -le 0) { $end = $i+1; break } }
        $i++
      }
      if ($end) { return $src.Remove($start, $end-$start) }
    }
    return $src
  }
  Edit-File $ed_cpp {
    param($s)
    # (viewport, flags[, nullptr]) -> (0, viewport, flags, nullptr)
    $s = [regex]::Replace($s, 'ImGui::DockSpaceOverViewport\s*\(\s*ImGui::GetMainViewport\(\)\s*,\s*([^\),]+?)(\s*,\s*nullptr)?\s*\)', 'ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), $1, nullptr)')
    # Any swapped variant -> canonical 4-arg order
    $s = [regex]::Replace($s, 'ImGui::DockSpaceOverViewport\s*\(\s*([_a-zA-Z0-9]+)\s*,\s*ImGui::GetMainViewport\(\)\s*,\s*([^\),]+?)\s*\)', 'ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), $2, nullptr)')
    return (Remove-SecondDrawUI $s)
  } | Out-Null
}

Write-Host "Patch pass complete."
