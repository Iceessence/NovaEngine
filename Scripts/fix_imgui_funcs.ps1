param(
  [Parameter(Mandatory=$true)][string]$RepoRoot
)
$ErrorActionPreference = "Stop"
$vk = Join-Path $RepoRoot "src/engine/renderer/vk/VulkanRenderer.cpp"
if (-not (Test-Path $vk)) { throw "File not found: $vk" }
$txt = Get-Content $vk -Raw
$orig = $txt

# Replace any CreateFontsTexture(cmd) to no-arg
$txt = $txt -replace 'ImGui_ImplVulkan_CreateFontsTexture\s*\(\s*[^)]*\s*\)', 'ImGui_ImplVulkan_CreateFontsTexture()'

# Ensure destroy call uses correct name
$txt = $txt -replace 'ImGui_ImplVulkan_InvalidateFontUploadObjects', 'ImGui_ImplVulkan_DestroyFontUploadObjects'

if ($txt -ne $orig) {
  Set-Content -Path $vk -Value $txt -NoNewline
  Write-Host "ImGui font upload API calls updated."
} else {
  Write-Host "No ImGui font API changes were necessary."
}
