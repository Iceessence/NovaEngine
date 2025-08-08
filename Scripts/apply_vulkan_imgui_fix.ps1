param(
  [Parameter(Mandatory=$true)][string]$RepoRoot
)
$ErrorActionPreference = "Stop"
function Edit-File($path, [scriptblock]$fn) {
  if (-not (Test-Path $path)) { throw "File not found: $path" }
  $text = Get-Content $path -Raw
  $new  = & $fn $text
  if ($new -ne $text) { Set-Content -Path $path -Value $new -Encoding UTF8; Write-Host "Patched: $path" }
  else { Write-Host "No changes: $path" }
}

# 1) VulkanRenderer.h — fix signatures and inline duplicates
$vrh = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.h"
if (Test-Path $vrh) {
  Edit-File $vrh {
    param($t)
    # Ensure forward declare of GLFWwindow if needed
    if ($t -notmatch "(?m)^\s*struct\s+GLFWwindow\s*;") {
      $t = $t -replace "(?ms)^(#pragma once\s*)", "`$1`nstruct GLFWwindow;`n"
    }
    # Fix InitImGui signature (add GLFWwindow*)
    $t = $t -replace "(?ms)\bvoid\s+InitImGui\s*\(\s*\)", "void InitImGui(GLFWwindow* window)"
    # Ensure BeginFrame/EndFrame declarations exist
    if ($t -notmatch "\bvoid\s+BeginFrame\s*\(\s*\)") {
      $t = $t -replace "(?ms)(class\s+VulkanRenderer\s*:[^{]*\{)", "`$1`npublic:`n    void BeginFrame();"
    }
    if ($t -notmatch "\bvoid\s+EndFrame\s*\(\s*VkCommandBuffer\s*\)") {
      $t = $t -replace "(?ms)(class\s+VulkanRenderer[^{]*\{[^}]*?public:[^}]*?)\K(?=})", "`n    void EndFrame(VkCommandBuffer cmd);"
    }
    # Remove inline IsImGuiReady body; leave only decl
    $t = $t -replace "(?ms)bool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{[^}]*\}", "bool IsImGuiReady() const;"
    if ($t -notmatch "\bbool\s+IsImGuiReady\s*\(\s*\)\s*const\s*;") {
      $t = $t -replace "(?ms)(class\s+VulkanRenderer[^{]*\{[^}]*?public:[^}]*?)\K(?=})", "`n    bool IsImGuiReady() const;"
    }
    # Ensure m_imguiReady member exists
    if ($t -notmatch "\bbool\s+m_imguiReady\b") {
      # add to private section if exists, else just before closing brace
      if ($t -match "(?ms)private:\s*") {
        $t = $t -replace "(?ms)(private:\s*)(?!.*m_imguiReady)", "`$1`n    bool m_imguiReady{false};"
      } else {
        $t = $t -replace "(?ms)\}\s*;\s*$", "private:`n    bool m_imguiReady{false};`n};"
      }
    }
    return $t
  }
} else {
  Write-Host "WARN: VulkanRenderer.h not found at $vrh"
}

# 2) VulkanRenderer.cpp — includes, signatures, helpers
$vrc = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.cpp"
if (Test-Path $vrc) {
  Edit-File $vrc {
    param($t)
    # Ensure volk and algorithm and helpers are included
    if ($t -notmatch "(?m)^\s*#\s*include\s*<volk\.h>\s*$") {
      $t = $t -replace "(?ms)(#include\s+<[^>]+vulkan[^>]*>\s*\r?\n)?", "#include <volk.h>`n"
    }
    if ($t -notmatch "(?m)^\s*#\s*include\s*<algorithm>\s*$") {
      $t = "#include <algorithm>`n" + $t
    }
    if ($t -notmatch "(?m)^\s*#\s*include\s*""VulkanHelpers\.h""\s*$") {
      # Try to include local helper for VK_CHECK
      $t = $t -replace "(?ms)(#include\s+""VulkanRenderer\.h""\s*)", "`$1`n#include `"VulkanHelpers.h`"`n"
    }
    # Fix InitImGui signature
    $t = $t -replace "(?ms)\bvoid\s+nova::VulkanRenderer::InitImGui\s*\(\s*\)", "void nova::VulkanRenderer::InitImGui(GLFWwindow* window)"
    # Guard IsImGuiReady duplicate out of cpp (we keep header decl only)
    $t = $t -replace "(?ms)^\s*bool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{[^}]*\}\s*", ""
    # Add this-> to member uses in InitImGui/BeginFrame/EndFrame lines for safety
    $t = $t -replace "(?m)^\s*(m_)", "this->`$1"
    $t = $t -replace "(?ms)(\W)m_(\w+)", "$1this->m_$2"
    return $t
  }
} else {
  Write-Host "WARN: VulkanRenderer.cpp not found at $vrc"
}

# 3) Editor.h — ensure DrawUI is declared once
$edh = Join-Path $RepoRoot "src/engine/editor/Editor.h"
if (Test-Path $edh) {
  Edit-File $edh {
    param($t)
    # Remove duplicate inline DrawUI bodies in header (should be declaration only)
    $t = $t -replace "(?ms)void\s+DrawUI\s*\(\s*\)\s*\{[^}]*\}", "void DrawUI();"
    if ($t -notmatch "\bvoid\s+DrawUI\s*\(\s*\)\s*;") {
      $t = $t -replace "(?ms)(class\s+Editor[^{]*\{[^}]*?public:[^}]*?)\K(?=})", "`n    void DrawUI();"
    }
    return $t
  }
} else {
  Write-Host "WARN: Editor.h not found at $edh"
}

# 4) Editor.cpp — fix DockSpaceOverViewport params & duplicate definitions
$edc = Join-Path $RepoRoot "src/engine/editor/Editor.cpp"
if (Test-Path $edc) {
  Edit-File $edc {
    param($t)
    # Keep first DrawUI definition; remove any additional duplicates
    $matches = [regex]::Matches($t, "(?ms)^\s*void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{.*?\}")
    if ($matches.Count -gt 1) {
      # keep first, remove others
      for ($i=1; $i -lt $matches.Count; $i++) {
        $t = $t.Remove($matches[$i].Index, $matches[$i].Length)
      }
    }
    # Fix DockSpaceOverViewport call: make first arg ImGuiID (0), second is viewport
    $t = $t -replace "ImGui::DockSpaceOverViewport\s*\(\s*ImGui::GetMainViewport\s*\(\s*\)\s*,", "ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),"
    $t = $t -replace "ImGui::DockSpaceOverViewport\s*\(\s*([_a-zA-Z0-9]+)\s*,\s*ImGuiDockNodeFlags_", "ImGui::DockSpaceOverViewport(0, `$1, ImGuiDockNodeFlags_"
    return $t
  }
} else {
  Write-Host "WARN: Editor.cpp not found at $edc"
}

# 5) CMakeLists.txt — ensure VK_NO_PROTOTYPES
$cm = Join-Path $RepoRoot "CMakeLists.txt"
if (Test-Path $cm) {
  Edit-File $cm {
    param($t)
    if ($t -notmatch "(?m)^\s*add_compile_definitions\s*\(\s*VK_NO_PROTOTYPES\s*\)\s*$") {
      # add near top after project()
      $t = $t -replace "(?ms)^(.*?project\s*\([^\)]*\)\s*\r?\n)", "$1add_compile_definitions(VK_NO_PROTOTYPES)`n"
    }
    return $t
  }
} else {
  Write-Host "WARN: CMakeLists.txt not found at $cm"
}

Write-Host ""
Write-Host "Done. Next:"
Write-Host "  cmake -S `"$RepoRoot`" -B `"$RepoRoot\build`" -DCMAKE_BUILD_TYPE=Debug"
Write-Host "  cmake --build `"$RepoRoot\build`" --config Debug"
Write-Host "  `"$RepoRoot\build\bin\Debug\NovaEditor.exe`""
