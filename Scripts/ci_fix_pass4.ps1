param([string]$RepoRoot='.')
$ErrorActionPreference = 'Stop'

function Edit-File($rel, [ScriptBlock]$editor) {
  $path = Join-Path $RepoRoot $rel
  if (!(Test-Path $path)) { Write-Host ("Skip (missing): {0}" -f $rel); return }
  $t = Get-Content -Raw $path
  $new = & $editor $t
  if ($new -ne $t) {
    Copy-Item $path ("$path.bak-{0}" -f (Get-Date -Format yyyyMMdd-HHmmss)) -Force
    Set-Content -Path $path -Value $new -Encoding UTF8
    Write-Host ("Patched: {0}" -f $rel)
  } else {
    Write-Host ("No changes: {0}" -f $rel)
  }
}

# --- VulkanRenderer.cpp: include imgui_impl_vulkan.h, add VK_CHECK, fix ImGui_ImplVulkan_Init call ---
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)

  if ($t -notmatch 'imgui_impl_vulkan\.h' -or $t -notmatch '\bVK_CHECK\b') {
    $inject = @"
#include "imgui_impl_vulkan.h"
#ifndef VK_CHECK
#include <cstdio>
#include <cstdlib>
#define VK_CHECK(x) do { VkResult _e=(x); if(_e!=VK_SUCCESS){ std::fprintf(stderr,"VK_CHECK failed %d at %s:%d\n",(int)_e,__FILE__,__LINE__); std::abort(); } } while(0)
#endif
"@
    $t = [regex]::Replace($t, '(?ms)(#include\s+"VulkanRenderer\.h".*?\r?\n)', '$1' + $inject + "`r`n", 1)
  }

  # Convert 2-arg ImGui_ImplVulkan_Init(&info, m_renderPass) -> info.RenderPass = m_renderPass; ImGui_ImplVulkan_Init(&info)
  $t = [regex]::Replace($t,
    'ImGui_ImplVulkan_Init\s*\(\s*&(?<info>[A-Za-z_]\w*)\s*,\s*(?<rp>[A-Za-z_]\w*)\s*\)',
    { param($m) ($m.Groups['info'].Value + '.RenderPass = ' + $m.Groups['rp'].Value + '; ImGui_ImplVulkan_Init(&' + $m.Groups['info'].Value + ')') }
  )

  # Remove duplicate IsImGuiReady() definitions in this TU (keep the first)
  $defs = [regex]::Matches($t, '(?ms)^\s*bool\s+nova::VulkanRenderer::IsImGuiReady\s*\([^)]*\)\s*(?:const\s*)?\{.*?\}', 'Multiline')
  if ($defs.Count -gt 1) { for ($i=1;$i -lt $defs.Count;$i++){ $t = $t.Remove($defs[$i].Index, $defs[$i].Length) } }

  return $t
}

# --- VulkanRenderer.h: ensure IsImGuiReady is just a declaration ---
Edit-File 'src/engine/renderer/vk/VulkanRenderer.h' {
  param($t)
  $t = [regex]::Replace($t, '(?ms)\bbool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{[^}]*\}', 'bool IsImGuiReady() const;')
  return $t
}

# --- Editor.cpp: de-dupe DrawUI and fix DockSpaceOverViewport argument order ---
Edit-File 'src/engine/editor/Editor.cpp' {
  param($t)

  # Remove duplicate DrawUI()
  $ms = [regex]::Matches($t,'(?ms)^\s*void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{.*?\}','Multiline')
  if ($ms.Count -gt 1) { for ($i=1;$i -lt $ms.Count;$i++){ $t = $t.Remove($ms[$i].Index, $ms[$i].Length) } }

  # Normalize DockSpaceOverViewport( viewport, flags, [wclass] ) -> DockSpaceOverViewport(0, viewport, flags, wclass/nullptr)
  $pattern = 'ImGui::DockSpaceOverViewport\s*\(\s*(?<a>[^,()\r\n]+)\s*,\s*(?<b>[^,()\r\n]+)(?:\s*,\s*(?<c>[^,()\r\n]+))?(?:\s*,\s*(?<d>[^,()\r\n]+))?\s*\)'
  $t = [regex]::Replace($t, $pattern, {
    param($m)
    $a = $m.Groups['a'].Value.Trim()
    $b = $m.Groups['b'].Value.Trim()
    $c = $m.Groups['c'].Value.Trim()
    $d = $m.Groups['d'].Value.Trim()
    if ($b -match '^(NULL|nullptr)$') { $flags = $c; $wclass = $b } else { $flags = $b; $wclass = (if ($c) { $c } else { 'nullptr' }) }
    "ImGui::DockSpaceOverViewport(0, $a, $flags, $wclass)"
  }, 'Singleline')

  return $t
}

Write-Host 'Patch applied.'
