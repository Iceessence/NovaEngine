param([string]$RepoRoot = ".")
Set-Location $RepoRoot

$path = 'src\engine\renderer\vk\VulkanRenderer.cpp'
if (!(Test-Path $path)) { Write-Host "Missing: $path"; exit 1 }
$t = Get-Content -Raw $path

# 1) Replace any "VkResult X = ImGui_ImplVulkan_Init(...);" (even if not at line start)
$t = [regex]::Replace(
  $t,
  '(?ms)\b(?:const\s+)?VkResult(?:\s+const)?\s+\w+\s*=\s*ImGui_ImplVulkan_Init\s*\(\s*([\s\S]*?)\)\s*;',
  'if (!ImGui_ImplVulkan_Init($1)) { std::fprintf(stderr,"ImGui_ImplVulkan_Init failed\n"); std::abort(); }'
)

# 1b) If they did a separate declaration then assignment:
$t = [regex]::Replace(
  $t,
  '(?ms)\bVkResult\s+(\w+)\s*;\s*([\s\S]*?)\b\1\s*=\s*ImGui_ImplVulkan_Init\s*\(\s*([\s\S]*?)\)\s*;',
  '$2 if (!ImGui_ImplVulkan_Init($3)) { std::fprintf(stderr,"ImGui_ImplVulkan_Init failed\n"); std::abort(); }'
)

# 2) Remove ALL existing IsImGuiReady() definitions in the cpp
$t = [regex]::Replace(
  $t,
  '(?ms)\bbool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*',
  ''
)

# 2b) Append exactly one correct definition if none present
if ($t -notmatch '(?ms)\bbool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{') {
  $append = "`r`nbool nova::VulkanRenderer::IsImGuiReady() const { return m_imguiReady; }`r`n"
  $t += $append
}

Set-Content -Path $path -Value $t -Encoding UTF8
Write-Host "Patched: $path"

# Build
cmake --build build --config Release
