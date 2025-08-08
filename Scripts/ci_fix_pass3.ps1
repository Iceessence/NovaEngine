param([string]$RepoRoot = ".")
$ErrorActionPreference = "Stop"

function Patch-File($path, [ScriptBlock]$patch) {
  if (-not (Test-Path $path)) { throw "Missing file: $path" }
  $orig = Get-Content $path -Raw
  $new  = & $patch $orig
  if ($new -ne $orig) {
    Copy-Item $path "$path.bak-$(Get-Date -Format yyyyMMdd-HHmmss)" -Force
    Set-Content $path $new -NoNewline
    Write-Host "Patched: $path"
  } else {
    Write-Host "No changes: $path"
  }
}

# 1) VulkanRenderer.h: make InitImGui non-static with GLFWwindow*; ensure IsImGuiReady inline
$vrh = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.h"
if (Test-Path $vrh) {
  Patch-File $vrh {
    param($t)
    $t = $t -replace '(?m)^\s*static\s+void\s+InitImGui\s*\([^)]*\)\s*;', 'void InitImGui(GLFWwindow* window);'
    $t = $t -replace '(?m)^\s*void\s+InitImGui\s*\(\s*\)\s*;', 'void InitImGui(GLFWwindow* window);'
    if ($t -notmatch '\bbool\s+IsImGuiReady\s*\(\s*\)\s+const') {
      $t = $t -replace '(?m)^(public:)', "`$1`r`n    inline bool IsImGuiReady() const { return m_imguiReady; }"
    }
    $t = $t -replace '(?m)^\s*static\s+(VkInstance|VkDevice|VkQueue|uint32_t|VkRenderPass|std::vector<\s*VkImage\s*>)', '$1'
    $t
  }
} else { Write-Warning "Missing $vrh"; }

# 2) VulkanRenderer.cpp: add <algorithm>, add VK_CHECK, fix ImGui init call, remove duplicate IsImGuiReady body
$vrc = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.cpp"
if (Test-Path $vrc) {
  Patch-File $vrc {
    param($t)
    if ($t -notmatch '(?m)^\s*#include\s*<algorithm>\s*$') {
      $t = $t -replace '(?m)^(#include\s*<.*?>\s*\r?\n)+', { $_.Value + "#include <algorithm>`r`n" }
    }
    if ($t -notmatch '\bVK_CHECK\b') {
      $t = $t -replace '(?s)(#include\s*".*?VulkanHelpers\.h".*?\r?\n)', '$1
#ifndef VK_CHECK
#define VK_CHECK(call) do { VkResult _vk_res = (call); if (_vk_res != VK_SUCCESS) { fprintf(stderr, "VK_CHECK failed %d at %s:%d\n", (int)_vk_res, __FILE__, __LINE__); abort(); } } while(0)
#endif

'
    }
    $t = $t -replace '(?m)^\s*void\s+nova::VulkanRenderer::InitImGui\s*\(\s*\)\s*', 'void nova::VulkanRenderer::InitImGui(GLFWwindow* window)'
    $t = $t -replace '(?m)^\s*static\s+void\s+nova::VulkanRenderer::InitImGui\s*\([^\)]*\)', 'void nova::VulkanRenderer::InitImGui(GLFWwindow* window)'
    $t = $t -replace 'ImGui_ImplVulkan_Init\s*\(\s*&?initInfo\s*,\s*m_renderPass\s*\)', 'initInfo.RenderPass = m_renderPass; ImGui_ImplVulkan_Init(&initInfo)'
    $t = $t -replace 'ImGui_ImplVulkan_Init\s*\(\s*initInfo\s*,\s*m_renderPass\s*\)', 'initInfo.RenderPass = m_renderPass; ImGui_ImplVulkan_Init(&initInfo)'
    $t = $t -replace '(?s)^\s*bool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*', ''
    $t
  }
} else { Write-Warning "Missing $vrc"; }

# 3) Editor.cpp: fix DockSpaceOverViewport signature + remove duplicate DrawUI body
$edc = Join-Path $RepoRoot "src/engine/editor/Editor.cpp"
if (Test-Path $edc) {
  Patch-File $edc {
    param($t)
    $t = $t -replace 'ImGui::DockSpaceOverViewport\s*\(\s*ImGui::GetMainViewport\s*\(\s*\)\s*,', 'ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(),'
    $t = $t -replace 'ImGui::DockSpaceOverViewport\s*\(\s*ImGui::GetMainViewport\s*\(\s*\)\s*\)', 'ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport())'
    $t = $t -replace '(?s)^\s*void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{.*?\}\s*(?=^\s*void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{)', ''
    $t
  }
} else { Write-Warning "Missing $edc"; }

Write-Host "Patch complete."