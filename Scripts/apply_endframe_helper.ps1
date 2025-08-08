param([string]$RepoRoot=".")
Set-Location $RepoRoot

function Edit-File([string]$path,[scriptblock]$sb){
  if(!(Test-Path $path)){ Write-Host "Skip: $path"; return }
  $t = Get-Content -Raw $path
  $n = & $sb $t
  if($n -ne $t){
    Set-Content -Path $path -Value $n -Encoding UTF8
    Write-Host "Patched: $path"
  } else {
    Write-Host "No changes: $path"
  }
}

# 1) VulkanRenderer.h — add GetActiveCmd() and a no-arg EndFrame() after the EndFrame(VkCommandBuffer) line
Edit-File 'src/engine/renderer/vk/VulkanRenderer.h' {
  param($t)
  $u = $t
  $needsGet = ($u -notmatch 'VkCommandBuffer\s+GetActiveCmd\s*\(\s*\)\s*const\s*;')
  $needsEnd0 = ($u -notmatch 'void\s+EndFrame\s*\(\s*\)\s*;')

  if($needsGet -or $needsEnd0){
    $inject = ""
    if($needsGet){ $inject += "    VkCommandBuffer GetActiveCmd() const;`r`n" }
    if($needsEnd0){ $inject += "    void EndFrame();`r`n" }

    # insert after the first EndFrame(VkCommandBuffer ...) declaration if present, else after 'public:'
    if($u -match '(?ms)(\bvoid\s+EndFrame\s*\(\s*VkCommandBuffer\s+\w+\s*\)\s*;\s*)'){
      $u = $u -replace '(?ms)(\bvoid\s+EndFrame\s*\(\s*VkCommandBuffer\s+\w+\s*\)\s*;\s*)', "`$1$inject", 1
    } else {
      $u = $u -replace '(?ms)(\bpublic:\s*\r?\n)', "`$1$inject", 1
    }
  }
  return $u
}

# 2) VulkanRenderer.cpp — define GetActiveCmd() and EndFrame() (no-arg) if missing
Edit-File 'src/engine/renderer/vk/VulkanRenderer.cpp' {
  param($t)
  $u = $t
  $needGet = ($u -notmatch 'VulkanRenderer::GetActiveCmd\s*\(')
  $needEnd = ($u -notmatch 'VulkanRenderer::EndFrame\s*\(\s*\)\s*\{')

  $append = ""
  if($needGet){
    $append += @"
VkCommandBuffer nova::VulkanRenderer::GetActiveCmd() const {
    if (m_cmds.empty() || m_frameIndex >= m_cmds.size()) return VK_NULL_HANDLE;
    return m_cmds[m_frameIndex];
}
"@
  }
  if($needEnd){
    $append += @"
void nova::VulkanRenderer::EndFrame() {
    VkCommandBuffer cmd = GetActiveCmd();
    if (cmd != VK_NULL_HANDLE) EndFrame(cmd);
}
"@
  }
  if($append -ne ""){
    $u = $u.TrimEnd() + "`r`n" + $append + "`r`n"
  }
  return $u
}

# 3) Build
cmake --build build --config Release | Out-Host

# 4) (Optional) run the exe
$exe = (Get-ChildItem -Recurse -File -Filter NovaEditor.exe .\build | Select-Object -First 1).FullName
if($exe){ & $exe }
