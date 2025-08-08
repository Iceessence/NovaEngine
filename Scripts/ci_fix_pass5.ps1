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

# --- VulkanRenderer.cpp: hard-inject include + VK_CHECK at file top, fix 2-arg ImGui init, dedupe IsImGuiReady ---
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)

  if ($t -notmatch 'imgui_impl_vulkan\.h') {
    $inject = @"
// ==== CI_PATCH ====
#include \"imgui_impl_vulkan.h\"
#ifndef VK_CHECK
#include <cstdio>
#include <cstdlib>
#define VK_CHECK(x) do { VkResult _e=(x); if(_e!=VK_SUCCESS){ std::fprintf(stderr,\"VK_CHECK failed %d at %s:%d\n\",(int)_e,__FILE__,__LINE__); std::abort(); } } while(0)
#endif
// ==== /CI_PATCH ====
"
    $t = $inject + "`r`n" + $t
  }

  # Replace: ImGui_ImplVulkan_Init(&info, m_renderPass);
  $t = [regex]::Replace(
    $t,
    'ImGui_ImplVulkan_Init\s*\(\s*&(?<info>[A-Za-z_]\w*)\s*,\s*(?<rp>[^)\s;]+)\s*\)\s*;',
    { param($m) ("{0}.RenderPass = {1}; ImGui_ImplVulkan_Init(&{0});" -f $m.Groups['info'].Value, $m.Groups['rp'].Value) },
    'Singleline'
  )

  # Remove duplicate IsImGuiReady definitions in this .cpp (keep the first)
  $defs = [regex]::Matches($t, '(?ms)^\s*bool\s+(?:nova::)?VulkanRenderer::IsImGuiReady\s*\([^)]*\)\s*(?:const\s*)?\{.*?\}', 'Multiline')
  if ($defs.Count -gt 1) {
    for ($i=1; $i -lt $defs.Count; $i++) {
      $t = $t.Remove($defs[$i].Index, $defs[$i].Length)
    }
  }

  return $t
}

# --- VulkanRenderer.h: ensure IsImGuiReady is declaration only (no inline body) ---
Edit-File 'src/engine/renderer/vk/VulkanRenderer.h' {
  param($t)
  $t = [regex]::Replace($t, '(?ms)\bbool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{[^}]*\}', 'bool IsImGuiReady() const;')
  return $t
}

# --- Editor.cpp: dedupe DrawUI + fix DockSpaceOverViewport signature(s) ---
Edit-File 'src/engine/editor/Editor.cpp' {
  param($t)

  # De-dupe DrawUI
  $ms = [regex]::Matches($t, '(?ms)^\s*void\s+(?:nova::)?Editor::DrawUI\s*\(\s*\)\s*\{.*?\}', 'Multiline')
  if ($ms.Count -gt 1) {
    for ($i=1; $i -lt $ms.Count; $i++) { $t = $t.Remove($ms[$i].Index, $ms[$i].Length) }
  }

  # Common bad form 1: DockSpaceOverViewport(ImGui::GetMainViewport(), flags, ...)
  $t = [regex]::Replace(
    $t,
    'ImGui::DockSpaceOverViewport\s*\(\s*ImGui::GetMainViewport\s*\(\s*\)\s*,\s*(?<flags>[^,\)\r\n]+)(?:\s*,\s*(?<rest>[^)\r\n]+))?\)',
    { param($m)
      $flags = $m.Groups['flags'].Value.Trim()
      $rest  = $m.Groups['rest'].Success ? (', ' + $m.Groups['rest'].Value.Trim()) : ', nullptr'
      "ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), $flags$rest)"
    },
    'Singleline'
  )

  # Generic reorder: if first arg looks like a viewport ptr and second is flags -> swap to (0, viewport, flags, wclass/nullptr)
  $t = [regex]::Replace(
    $t,
    'ImGui::DockSpaceOverViewport\s*\(\s*(?<a>[^,()\r\n]+)\s*,\s*(?<b>[^,()\r\n]+)(?:\s*,\s*(?<c>[^,()\r\n]+))?(?:\s*,\s*(?<d>[^,()\r\n]+))?\s*\)',
    {
      param($m)
      $a=$m.Groups['a'].Value.Trim(); $b=$m.Groups['b'].Value.Trim()
      $c=$m.Groups['c'].Success ? $m.Groups['c'].Value.Trim() : $null
      $d=$m.Groups['d'].Success ? $m.Groups['d'].Value.Trim() : $null
      if ($a -match 'GetMainViewport' -or $a -match 'ImGuiViewport\s*\*') {
        $flags = $b
        $wclass = ($c ? $c : 'nullptr')
        "ImGui::DockSpaceOverViewport(0, $a, $flags, $wclass)"
      } else {
        $m.Value  # leave untouched
      }
    },
    'Singleline'
  )

  return $t
}

Write-Host 'Force patch applied.'
