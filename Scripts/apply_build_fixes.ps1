param([string]$RepoRoot = ".")
Set-Location $RepoRoot

function Edit-File([string]$path, [ScriptBlock]$edit) {
  if (!(Test-Path $path)) { Write-Host "Skip (missing): $path"; return }
  Copy-Item $path "$path.bak-$(Get-Date -Format yyyyMMdd-HHmmss)" -Force
  $t = Get-Content -Raw -Path $path
  $n = & $edit $t
  if ($n -ne $t) {
    Set-Content -Path $path -Value $n -Encoding UTF8
    Write-Host "Patched: $path"
  } else {
    Write-Host "No changes: $path"
  }
}

# 1) VulkanRenderer.h — single decl for InitImGui, make IsImGuiReady a declaration (no inline body)
Edit-File 'src/engine/renderer/vk/VulkanRenderer.h' {
  param($t)
  $u = $t
  # remove ALL InitImGui decls
  $u = [regex]::Replace($u, '(?ms)^\s*void\s+InitImGui\s*\(\s*GLFWwindow\s*\*\s*\w*\s*\)\s*;\s*\r?\n', '')
  # IsImGuiReady: strip inline bodies to pure declaration
  $u = [regex]::Replace($u, '(?ms)\bbool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*', 'bool IsImGuiReady() const;' + "`r`n")
  # ensure exactly one InitImGui decl after first 'public:'
  if ($u -notmatch 'void\s+InitImGui\s*\(\s*GLFWwindow\s*\*') {
    $u = [regex]::Replace($u, '(?ms)(\bpublic:\s*\r?\n)', '$1    void InitImGui(GLFWwindow* window);' + "`r`n", 1)
  }
  return $u
}

# 2) VulkanRenderer.cpp — ensure include, VK_CHECK, fix bool/ImGui init, dedupe IsImGuiReady definition
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)
  $u = $t

  # include imgui_impl_vulkan.h if missing
  if ($u -notmatch 'imgui_impl_vulkan\.h') {
    $u = [regex]::Replace($u, '(?ms)(#include\s+"VulkanRenderer\.h"\s*\r?\n)', '$1#include "imgui_impl_vulkan.h"' + "`r`n", 1)
  }
  # <cstdio> for fprintf
  if ($u -notmatch '<cstdio>') {
    $u = [regex]::Replace($u, '(?ms)(#include\s+<[^>]+>\s*\r?\n)', '$1#include <cstdio>' + "`r`n", 1)
  }

  # define VK_CHECK once after includes
  if ($u -notmatch '\bVK_CHECK\b') {
$vk = @'
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult __res = (x); if (__res != VK_SUCCESS) { std::fprintf(stderr,"VK_CHECK failed: %d\n",(int)__res); std::abort(); } } while(0)
#endif

