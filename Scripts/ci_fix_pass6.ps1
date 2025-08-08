param([string]$RepoRoot='.')
$ErrorActionPreference = 'Stop'

function Edit-File { param([string]$rel,[ScriptBlock]$fn)
  $p = Join-Path $RepoRoot $rel
  if(!(Test-Path $p)){ Write-Host "Skip (missing): $rel"; return }
  $t = Get-Content -Raw $p
  $n = & $fn $t
  if($n -ne $t){
    Copy-Item $p "$p.bak-$(Get-Date -Format yyyyMMdd-HHmmss)" -Force
    Set-Content -Path $p -Value $n -Encoding UTF8
    Write-Host "Patched: $rel"
  } else { Write-Host "No changes: $rel" }
}

# --- VulkanRenderer.h: keep ONE InitImGui decl; make IsImGuiReady a decl only ---
Edit-File 'src/engine/renderer/vk/VulkanRenderer.h' {
  param($t)
  # ensure IsImGuiReady is not inline-defined
  $t = [regex]::Replace($t,'(?ms)\bbool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{[^}]*\}','bool IsImGuiReady() const;')
  # collapse ANY number of duplicate InitImGui decls into a single canonical line
  $t = [regex]::Replace($t,'(?ms)(?:^\s*void\s+InitImGui\s*\(\s*GLFWwindow\s*\*\s*\w*\s*\)\s*;\s*)+','void InitImGui(GLFWwindow* window);\r\n')
  return $t
}

# --- VulkanRenderer.cpp: include imgui_impl_vulkan.h, only bool-check ImGui init, not VK_CHECK ---
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)

  if($t -notmatch 'imgui_impl_vulkan\.h'){
    $header = @"
#include "imgui_impl_vulkan.h"
#ifndef VK_CHECK
#include <cstdio>
#include <cstdlib>
#define VK_CHECK(x) do { VkResult _e=(x); if(_e!=VK_SUCCESS){ std::fprintf(stderr,"VK_CHECK failed %d at %s:%d\n",(int)_e,__FILE__,__LINE__); std::abort(); } } while(0)
#endif

"@
    $t = $header + $t
  }

  # Convert 2-arg ImGui_ImplVulkan_Init(&info, m_renderPass) -> 1-arg + bool check
  $t = [regex]::Replace(
    $t,
    'ImGui_ImplVulkan_Init\s*\(\s*&(?<info>[A-Za-z_]\w*)\s*,\s*(?<rp>[^)\s;]+)\s*\)\s*;',
    { param($m) ("{0}.RenderPass = {1}; if (!ImGui_ImplVulkan_Init(&{0})) {{ std::fprintf(stderr,\"ImGui_ImplVulkan_Init failed\n\"); std::abort(); }}" -f $m.Groups['info'].Value, $m.Groups['rp'].Value) },
    'Singleline'
  )

  # Strip VK_CHECK around ImGui init entirely -> bool check
  $t = [regex]::Replace(
    $t,
    'VK_CHECK\s*\(\s*ImGui_ImplVulkan_Init\s*\(\s*(?<args>[^)]*)\)\s*\)\s*;',
    { param($m) ("if (!ImGui_ImplVulkan_Init({0})) {{ std::fprintf(stderr,\"ImGui_ImplVulkan_Init failed\n\"); std::abort(); }}" -f $m.Groups['args'].Value) },
    'Singleline'
  )

  # Replace "VkResult x = ImGui_ImplVulkan_Init(...);" -> bool + check
  $t = [regex]::Replace(
    $t,
    'VkResult\s+[A-Za-z_]\w*\s*=\s*ImGui_ImplVulkan_Init\s*\(\s*(?<args>[^)]*)\)\s*;',
    { param($m) ("bool __imgui_ok = ImGui_ImplVulkan_Init({0}); if (!__imgui_ok) {{ std::fprintf(stderr,\"ImGui_ImplVulkan_Init failed\n\"); std::abort(); }}" -f $m.Groups['args'].Value) },
    'Singleline'
  )

  # De-dupe any extra IsImGuiReady definitions in this cpp
  $defs = [regex]::Matches($t,'(?ms)^\s*bool\s+(?:nova::)?VulkanRenderer::IsImGuiReady\s*\([^)]*\)\s*(?:const\s*)?\{.*?\}','Multiline')
  if($defs.Count -gt 1){
    for($i=1;$i -lt $defs.Count;$i++){ $t = $t.Remove($defs[$i].Index,$defs[$i].Length) }
  }

  return $t
}

# --- Editor.cpp: remove duplicate DrawUI and normalize DockSpaceOverViewport ---
Edit-File 'src/engine/editor/Editor.cpp' {
  param($t)
  # Keep first DrawUI, remove the rest
  $defs = [regex]::Matches($t,'(?ms)^\s*void\s+(?:nova::)?Editor::DrawUI\s*\(\s*\)\s*\{.*?\}','Multiline')
  if($defs.Count -gt 1){
    for($i=1;$i -lt $defs.Count;$i++){ $t = $t.Remove($defs[$i].Index,$defs[$i].Length) }
  }

  # Normalize DockSpaceOverViewport(0, GetMainViewport(), flags, nullptr)
  $t = [regex]::Replace(
    $t,
    'ImGui::DockSpaceOverViewport\s*\(\s*ImGui::GetMainViewport\s*\(\s*\)\s*(?:,\s*)?(?<rest>[^)]*)\)',
    {
      param($m)
      $rest = $m.Groups['rest'].Value
      $flags = '0'
      if($rest){
        $parts = $rest -split '\s*,\s*'
        if($parts.Count -ge 1){ $flags = $parts[0] }
      }
      "ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), $flags, nullptr)"
    },
    'Singleline'
  )
  return $t
}
