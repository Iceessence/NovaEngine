param([string]$RepoRoot = ".")
Set-Location $RepoRoot

function Edit-File([string]$p, [ScriptBlock]$fn) {
  if (!(Test-Path $p)) { Write-Host "Skip (missing): $p"; return }
  $t = Get-Content -Raw $p
  $n = & $fn $t
  if ($n -ne $t) { Set-Content $p -Value $n -Encoding UTF8; Write-Host "Patched: $p" } else { Write-Host "No changes: $p" }
}

# ---- VulkanRenderer.cpp: fix ImGui init assignment and dedupe IsImGuiReady ----
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)
  $u = $t

  # Replace ANY pattern like "const VkResult ... = ImGui_ImplVulkan_Init(...);"
  $u = [regex]::Replace(
    $u,
    '(?ms)\b(?:const\s+)?VkResult(?:\s+const)?\s+\w+\s*=\s*ImGui_ImplVulkan_Init\s*\(\s*([\s\S]*?)\)\s*;',
    'if (!ImGui_ImplVulkan_Init($1)) { std::fprintf(stderr,"ImGui_ImplVulkan_Init failed\n"); std::abort(); }'
  )

  # Remove ALL existing IsImGuiReady() definitions in this file
  $u = [regex]::Replace(
    $u,
    '(?ms)\bbool\s+[A-Za-z_]\w*::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}\s*',
    ''
  )

  # Ensure exactly one definition remains (append at end if missing)
  if ($u -notmatch '(?ms)\bbool\s+[A-Za-z_]\w*::IsImGuiReady\s*\(\s*\)\s*const\s*\{') {
    $u += "`r`nbool nova::VulkanRenderer::IsImGuiReady() const { return m_imguiReady; }`r`n"
  }

  return $u
}

Write-Host "Pass2 patch done."
