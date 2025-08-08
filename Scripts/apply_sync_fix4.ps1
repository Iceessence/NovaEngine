
param(
  [Parameter(Mandatory=$true)][string]$RepoRoot
)

function Read-File($p) { return [System.IO.File]::ReadAllText($p) }
function Write-File($p,$c) { [System.IO.File]::WriteAllText($p, $c, (New-Object System.Text.UTF8Encoding($false))) }
function Replace-In-File {
  param([string]$path, [string]$pattern, [string]$replace)
  $t = Read-File $path
  $new = [Regex]::Replace($t, $pattern, $replace, 'Singleline')
  if ($new -ne $t) { Write-File $path $new; return $true } else { return $false }
}

# 1) VulkanRenderer.cpp: ensure volk-first include and <algorithm>
$vr_cpp = Join-Path $RepoRoot "src\engine\renderer\vk\VulkanRenderer.cpp"
if (Test-Path $vr_cpp) {
  $t = Read-File $vr_cpp

  # Remove any direct vulkan.h includes (we'll rely on volk.h)
  $t2 = [Regex]::Replace($t, '(?m)^\s*#\s*include\s*<vulkan\/vulkan\.h>\s*\r?\n', '')

  # Ensure header prologue
  $prologue = @"
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <volk.h>
"@

  if ($t2 -notmatch '(?m)^\s*#\s*include\s*<volk\.h>') {
    # insert prologue before first include
    $t2 = [Regex]::Replace($t2, '(?s)\A', $prologue + "`r`n")
  }

  # Ensure <algorithm> for std::transform and friends
  if ($t2 -notmatch '(?m)^#\s*include\s*<algorithm>') {
    # add after first block of includes
    $t2 = [Regex]::Replace($t2, '(?m)(^#\s*include\s*<.*?>\s*\r?\n)+', { param($m) $m.Value + "#include <algorithm>`r`n" }, 1)
  }

  # Provide a safe VK_CHECK if missing
  if ($t2 -notmatch '\bVK_CHECK\b') {
    $vkcheck = @'
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _res = (x); if (_res != VK_SUCCESS) { /* TODO: add logging */ } } while(0)
#endif
'@
    $t2 = $t2.replace("#include <algorithm>\n", "#include <algorithm>\n" + $vkcheck + "\n")
  }

  if ($t2 -ne $t) { Write-File $vr_cpp $t2; Write-Output "Patched volk/includes in VulkanRenderer.cpp" } else { Write-Output "No changes: VulkanRenderer.cpp" }
} else { Write-Output "Missing: $vr_cpp" }

# 2) VulkanRenderer.h: add declarations and members if absent
$vr_h = Join-Path $RepoRoot "src\engine\renderer\vk\VulkanRenderer.h"
if (Test-Path $vr_h) {
  $t = Read-File $vr_h
  $changed = $false

  if ($t -notmatch '\bbool\s+m_imguiReady\b') {
    $t = [Regex]::Replace($t, '(?s)\};\s*\Z', "private:`r`n    bool m_imguiReady = false;`r`n    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;`r`npublic:`r`n    bool IsImGuiReady() const { return m_imguiReady; }`r`n    void InitImGui(VkRenderPass renderPass);`r`n    void BeginFrame();`r`n    void EndFrame(VkCommandBuffer cmd);`r`n};`r`n")
    $changed = $true
  } else {
    foreach ($meth in @('IsImGuiReady','InitImGui','BeginFrame','EndFrame')) {
      if ($t -notmatch [Regex]::Escape($meth)) { $changed = $true }
    }
    if ($changed) {
      $t = [Regex]::Replace($t, '(?s)\};\s*\Z', "public:`r`n    bool IsImGuiReady() const { return m_imguiReady; }`r`n    void InitImGui(VkRenderPass renderPass);`r`n    void BeginFrame();`r`n    void EndFrame(VkCommandBuffer cmd);`r`n};`r`n")
    }
  }

  if ($changed) { Write-File $vr_h $t; Write-Output "Patched: VulkanRenderer.h" } else { Write-Output "No changes: VulkanRenderer.h" }
} else { Write-Output "Missing: $vr_h" }

# 3) Editor.h: ensure DrawUI is declared
$ed_h = Join-Path $RepoRoot "src\engine\editor\Editor.h"
if (Test-Path $ed_h) {
  $t = Read-File $ed_h
  if ($t -notmatch '\bvoid\s+DrawUI\s*\(') {
    $t = [Regex]::Replace($t, '(?s)\};\s*\Z', "public:`r`n    void DrawUI();`r`n};`r`n")
    Write-File $ed_h $t
    Write-Output "Patched: Editor.h (DrawUI declaration)"
  } else {
    Write-Output "No changes: Editor.h"
  }
} else { Write-Output "Missing: $ed_h" }

# 4) Editor.cpp: if no DrawUI definition, append a minimal one
$ed_cpp = Join-Path $RepoRoot "src\engine\editor\Editor.cpp"
if (Test-Path $ed_cpp) {
  $t = Read-File $ed_cpp
  if ($t -notmatch '(?ms)^\s*void\s+nova::Editor::DrawUI\s*\(') {
    $stub = @'
// --- Added by apply_sync_fix4.ps1 ---
#include "imgui.h"
namespace nova {
void Editor::DrawUI() {
#ifdef IMGUI_HAS_DOCK
    ImGuiDockNodeFlags dock_flags = ImGuiDockNodeFlags_None;
    #if defined(IMGUI_VERSION_NUM) && (IMGUI_VERSION_NUM >= 18900)
        ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), dock_flags);
    #else
        ImGuiID dockspace_id = ImGui::GetID("DockSpace");
        ImGui::DockSpace(dockspace_id, ImVec2(0,0), dock_flags);
    #endif
#else
    // No docking in this build
#endif
}
} // namespace nova
'@
    Add-Content -Path $ed_cpp -Value $stub
    Write-Output "Appended DrawUI() stub to Editor.cpp"
  } else {
    Write-Output "No changes: Editor.cpp (DrawUI exists)"
  }
} else { Write-Output "Missing: $ed_cpp" }

Write-Output "`nDone. Now build:"
Write-Output "  cmake --build build --config Debug"
Write-Output "  .\build\bin\Debug\NovaEditor.exe"
