param([string]$RepoRoot = ".")
$ErrorActionPreference = "Stop"

function Backup($path) {
  if (Test-Path $path) {
    Copy-Item $path "$path.bak-$(Get-Date -Format yyyyMMdd-HHmmss)" -Force
  }
}

function Edit-Text($file, [scriptblock]$mutator) {
  $t = Get-Content $file -Raw -ErrorAction Stop
  $orig = $t
  $t = & $mutator $t
  if ($t -ne $orig) { Set-Content $file $t -Encoding utf8 }
  return $t -ne $orig
}

# --- Files ---
$vrh = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.h'
$vrc = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.cpp'
$edc = Join-Path $RepoRoot 'src\engine\editor\Editor.cpp'

# --- 1) VulkanRenderer.h: ensure InitImGui signature + single IsImGuiReady decl ---
if (Test-Path $vrh) {
  Backup $vrh
  Edit-Text $vrh {
    param($t)
    # Make InitImGui take GLFWwindow* (header/cpp mismatch fix)
    $t = $t -replace '(?m)^\s*void\s+InitImGui\s*\(\s*\)\s*;', 'void InitImGui(GLFWwindow* window);'
    # If it's missing entirely, add under first "public:" in class
    if ($t -notmatch 'InitImGui\s*\(\s*GLFWwindow\s*\*\s*window\s*\)') {
      $t = $t -replace '(?ms)(class\s+VulkanRenderer\s*:\s*public\s+[^\{]+\{.*?public:\s*)',
        '$1' + "`r`n    void InitImGui(GLFWwindow* window);`r`n"
    }
    # Ensure IsImGuiReady is declaration only in the header (no inline body)
    $t = $t -replace '(?ms)bool\s+IsImGuiReady\s*\(\s*\)\s*const\s*\{\s*return\s+[^\}]*\}', 'bool IsImGuiReady() const;'
    $t
  } | Out-Null
}

# --- 2) VulkanRenderer.cpp ---
if (Test-Path $vrc) {
  Backup $vrc
  Edit-Text $vrc {
    param($t)

    # Include helper header so VK types/macros resolve; also define VK_CHECK if missing
    if ($t -notmatch '(?m)^\s*#include\s+"VulkanHelpers\.h"') {
      $t = $t -replace '(?m)^(#include\s+<imgui_impl_vulkan\.h>.*?$)',
        "`$1`r`n#include `"VulkanHelpers.h`""
    }
    if ($t -notmatch '(?ms)^\s*#ifndef\s+VK_CHECK\b') {
      $t = $t -replace '(?m)^(#include\s+"VulkanHelpers\.h".*?$)',
        '$1' + "`r`n`r`n#ifndef VK_CHECK`r`n#define VK_CHECK(x) do { VkResult __e=(x); if (__e!=VK_SUCCESS) { fprintf(stderr, ""VK error %d at %s:%d\n"", __e, __FILE__, __LINE__); abort(); } } while(0)`r`n#endif"
    }

    # Remove any duplicate cpp definition of IsImGuiReady (we keep header decl or one cpp impl)
    $t = $t -replace '(?ms)^\s*bool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*', ''

    # Put a single simple cpp implementation back (safe)
    if ($t -notmatch '(?ms)^\s*bool\s+nova::VulkanRenderer::IsImGuiReady') {
      $t += "`r`nbool nova::VulkanRenderer::IsImGuiReady() const { return m_imguiReady; }`r`n"
    }

    # Replace old two-arg ImGui_ImplVulkan_Init(...) with new struct-based init
    $t = $t -replace '(?ms)ImGui_ImplVulkan_Init\s*\([^;]*?\);\s*',
@"
{
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance       = m_instance;
    init_info.PhysicalDevice = m_phys;
    init_info.Device         = m_dev;
    init_info.QueueFamily    = m_queueFamily;
    init_info.Queue          = m_queue;
    init_info.DescriptorPool = m_imguiPool;
    init_info.MinImageCount  = (uint32_t) (m_images.size() > 0 ? m_images.size() : 2);
    init_info.ImageCount     = init_info.MinImageCount;
    init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    init_info.Subpass        = 0;
    init_info.UseDynamicRendering = VK_FALSE;
    // init_info.CheckVkResultFn = [](VkResult r){ if(r) fprintf(stderr, "VK err %d\n", r); };

    VK_CHECK(ImGui_ImplVulkan_Init(&init_info, m_renderPass));
}
"@

    $t
  } | Out-Null
}

# --- 3) Editor.cpp: keep only one DrawUI() and fix DockSpaceOverViewport() call ---
if (Test-Path $edc) {
  Backup $edc
  Edit-Text $edc {
    param($t)

    # Remove all but first DrawUI definition
    $matches = [System.Text.RegularExpressions.Regex]::Matches($t, 'void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{', 'Singleline')
    if ($matches.Count -gt 1) {
      $firstIndex = $matches[0].Index
      # naive block stripper: keep from start to end of first body, then delete other bodies
      # Find end of first body by counting braces from the first '{'
      $openIdx = $t.IndexOf('{', $firstIndex)
      $balance = 0; $endIdx = $openIdx
      for ($i=$openIdx; $i -lt $t.Length; $i++) {
        if ($t[$i] -eq '{') { $balance++ }
        elseif ($t[$i] -eq '}') { $balance-- }
        if ($balance -eq 0 -and $i -gt $openIdx) { $endIdx = $i; break }
      }
      $before = $t.Substring(0, $endIdx+1)
      $after  = $t.Substring($endIdx+1)
      # Kill any later DrawUI definitions
      $after = [regex]::Replace($after, '(?ms)\s*void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{.*?\}', '')
      $t = $before + $after
    }

    # Normalize DockSpaceOverViewport call to (viewport, flags, window_class)
    $t = $t -replace '(?ms)ImGui::DockSpaceOverViewport\s*\([^;]+?\);',
      'ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode | ImGuiDockNodeFlags_AutoHideTabBar, nullptr);'

    $t
  } | Out-Null
}

Write-Host "Applied Vulkan/ImGui/Editor fixes."
