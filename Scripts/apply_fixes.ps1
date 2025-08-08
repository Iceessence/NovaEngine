param([string]$RepoRoot = ".")
Set-Location $RepoRoot

function Backup-File([string]$path) {
  if (Test-Path $path) {
    $stamp = Get-Date -Format "yyyyMMdd-HHmmss"
    Copy-Item $path "$path.bak-$stamp" -Force
  }
}

function Edit-File([string]$path, [ScriptBlock]$edit) {
  if (!(Test-Path $path)) { Write-Host "Skip (missing): $path"; return }
  Backup-File $path
  $t = Get-Content -Raw -Path $path
  $n = & $edit $t
  if ($n -ne $t) { Set-Content -Path $path -Value $n -Encoding UTF8; Write-Host "Patched: $path" } else { Write-Host "No changes: $path" }
}

# ----------------------------
# 1) VulkanRenderer.h — keep ONE InitImGui decl; make IsImGuiReady a declaration
# ----------------------------
Edit-File 'src/engine/renderer/vk/VulkanRenderer.h' {
  param($t)
  $u = $t
  # remove ALL InitImGui declarations
  $u = [regex]::Replace($u, '(?ms)^\s*void\s+InitImGui\s*\(\s*GLFWwindow\s*\*\s*\w*\s*\)\s*;\s*\r?\n', '')
  # ensure IsImGuiReady() is only declared
  $u = [regex]::Replace($u, '(?ms)\bbool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*', 'bool IsImGuiReady() const;' + "`r`n")
  # inject a single InitImGui if missing (after the first 'public:')
  if ($u -notmatch 'void\s+InitImGui\s*\(\s*GLFWwindow\s*\*') {
    $u = [regex]::Replace($u, '(?ms)(\bpublic:\s*\r?\n)', '$1    void InitImGui(GLFWwindow* window);' + "`r`n", 1)
  }
  return $u
}

# ----------------------------
# 2) VulkanRenderer.cpp — include, VK_CHECK, fix ImGui init(bool), ensure one IsImGuiReady()
# ----------------------------
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)
  $u = $t

  # ensure backend header
  if ($u -notmatch 'imgui_impl_vulkan\.h') {
    $u = [regex]::Replace($u, '(?ms)(#include\s+"VulkanRenderer\.h"\s*\r?\n)', '$1#include "imgui_impl_vulkan.h"' + "`r`n", 1)
  }

  # ensure <cstdio> for fprintf
  if ($u -notmatch '(?m)^#include\s*<cstdio>') {
    $u = [regex]::Replace($u, '(?ms)^(#include[^\r\n]+\r?\n+)', '$1#include <cstdio>' + "`r`n", 1)
  }

  # add VK_CHECK once (after includes)
  if ($u -notmatch '\bVK_CHECK\b') {
$vk = @'
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult __res = (x); if (__res != VK_SUCCESS) { std::fprintf(stderr,"VK_CHECK failed: %d\n",(int)__res); std::abort(); } } while(0)
#endif

'@
    $u = [regex]::Replace($u, '(?ms)^(#include[^\r\n]+\r?\n(?:#include[^\r\n]+\r?\n)*)', "`$1$vk", 1)
  }

  # fix: VkResult var = ImGui_ImplVulkan_Init(...); -> bool check
  $u = [regex]::Replace(
    $u,
    '(?m)^\s*VkResult\s+\w+\s*=\s*ImGui_ImplVulkan_Init\(([^)]*)\)\s*;\s*',
    'if (!ImGui_ImplVulkan_Init($1)) { std::fprintf(stderr,"ImGui_ImplVulkan_Init failed\n"); std::abort(); }'
  )

  # fix: old 2-arg call -> 1-arg
  $u = [regex]::Replace(
    $u,
    'ImGui_ImplVulkan_Init\s*\(\s*([^,\)]+)\s*,\s*[^)]+\)',
    'ImGui_ImplVulkan_Init($1)'
  )

  # nuke ALL IsImGuiReady() defs in cpp, then add one at end
  $u = [regex]::Replace($u, '(?ms)^\s*bool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*', '')
  if ($u -notmatch '(?ms)\bbool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{') {
$add = @'
bool nova::VulkanRenderer::IsImGuiReady() const {
    return m_imguiReady;
}

'@
    $u += $add
  }

  return $u
}

# ----------------------------
# 3) Editor.cpp — remove dup DrawUI; normalize DockSpace; add minimal DrawUI
# ----------------------------
Edit-File 'src/engine/editor/Editor.cpp' {
  param($t)
  $u = $t

  # remove ALL existing DrawUI() definitions
  $u = [regex]::Replace($u, '(?ms)^\s*void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{.*?\}\s*', '')

  # normalize ANY DockSpaceOverViewport call (ensure trailing semicolon is kept)
  $u = [regex]::Replace(
    $u,
    'ImGui::DockSpaceOverViewport\s*\([\s\S]*?\)\s*;',
    'ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);'
  )

$append = @'
void nova::Editor::DrawUI() {
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);
    ImGui::Render();
}

'@
  $u += $append
  return $u
}

Write-Host "Source patched."

# ----------------------------
# 4) Configure & build
# ----------------------------
if (!(Test-Path 'build')) {
  cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release | Out-Host
}
cmake --build build --config Release
