
param(
  [string]$RepoRoot = "."
)

$ErrorActionPreference = "Stop"

function Write-Note($msg){ Write-Host "  - $msg" -ForegroundColor Cyan }

function Save-Text([string]$Path,[string]$Text){
  $dir = Split-Path -Parent $Path
  if(!(Test-Path $dir)){ New-Item -ItemType Directory -Path $dir | Out-Null }
  Set-Content -Path $Path -Value $Text -Encoding UTF8 -NoNewline
}

function Patch-CMakeLists {
  param([string]$File)
  if(!(Test-Path $File)){ return $false }
  $t = Get-Content $File -Raw
  if($t -match 'VK_NO_PROTOTYPES'){ return $false }
  if($t -match '(?ms)(project\([^\)]*\)\s*)'){
    $t = $t -replace '(?ms)(project\([^\)]*\)\s*)', '$1`n# Ensure volk is used as the Vulkan loader across all TUs`nadd_compile_definitions(VK_NO_PROTOTYPES)`n'
  } else {
    $t = "# Ensure volk is used as the Vulkan loader across all TUs`nadd_compile_definitions(VK_NO_PROTOTYPES)`n`n" + $t
  }
  Save-Text -Path $File -Text $t
  return $true
}

function Patch-VulkanRendererCpp {
  param([string]$File)
  if(!(Test-Path $File)){ return $false }
  $t = Get-Content $File -Raw
  $changed = $false

  # Ensure volk.h goes first and VK_NO_PROTOTYPES is defined before any Vulkan headers
  if($t -notmatch 'volk\.h'){
    $prefix = @"
#if !defined(VK_NO_PROTOTYPES)
#define VK_NO_PROTOTYPES
#endif
#include <volk.h>

"@
    $t = $prefix + $t
    $changed = $true
  } else {
    # Make sure the define is before any includes
    if($t -notmatch 'VK_NO_PROTOTYPES'){
      $t = $t -replace '(^\s*#include\s*<volk\.h>\s*)', "`n#if !defined(VK_NO_PROTOTYPES)`n#define VK_NO_PROTOTYPES`n#endif`n$1"
      $changed = $true
    }
  }

  # Ensure <algorithm> is available for std::transform etc.
  if($t -notmatch '<algorithm>'){
    $t = $t -replace '(^\s*#include[^\n]*\n)', '$1#include <algorithm>' + "`n"
    $changed = $true
  }

  # Provide a minimal VK_CHECK if missing
  if($t -notmatch 'VK_CHECK\('){
    $vkcheck = @"
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _err = (x); if (_err != VK_SUCCESS) { /* TODO: hook to your logger */ __debugbreak(); } } while(0)
#endif

"@
    # insert after volk include block
    if($t -match 'volk\.h>'){
      $t = $t -replace '(#include\s*<volk\.h>\s*\r?\n)', "$1$vkcheck"
    } else {
      $t = $vkcheck + $t
    }
    $changed = $true
  }

  Save-Text -Path $File -Text $t
  return $changed
}

function Replace-EditorHeader {
  param([string]$File)
  $desired = @"
#pragma once

namespace nova {

class Editor {
public:
    void DrawUI();
};

} // namespace nova
"@
  if(!(Test-Path $File)){
    Save-Text -Path $File -Text $desired
    return $true
  }
  $cur = Get-Content $File -Raw
  if($cur -ne $desired){
    Copy-Item $File "$File.bak" -Force
    Save-Text -Path $File -Text $desired
    return $true
  }
  return $false
}

# ---- Drive patching ----

$repo = Resolve-Path $RepoRoot
Write-Host "Applying sync-fix 3 to $repo" -ForegroundColor Green

$cmake = Join-Path $repo "CMakeLists.txt"
if(Patch-CMakeLists $cmake){
  Write-Note "CMakeLists.txt: added add_compile_definitions(VK_NO_PROTOTYPES)"
}else{
  Write-Note "CMakeLists.txt: no change needed"
}

$vkcpp = Join-Path $repo "src/engine/renderer/vk/VulkanRenderer.cpp"
if(Patch-VulkanRendererCpp $vkcpp){
  Write-Note "VulkanRenderer.cpp: ensured VK_NO_PROTOTYPES + <volk.h> first, added <algorithm>, VK_CHECK"
}else{
  Write-Note "VulkanRenderer.cpp: no change needed"
}

$editorh = Join-Path $repo "src/engine/editor/Editor.h"
if(Replace-EditorHeader $editorh){
  Write-Note "Editor.h replaced with a clean minimal declaration (backup at Editor.h.bak)"
}else{
  Write-Note "Editor.h already in desired shape"
}

Write-Host "`nDone. Now rebuild:" -ForegroundColor Yellow
Write-Host "  cmake --build build --config Debug" -ForegroundColor Yellow
Write-Host "  .\build\bin\Debug\NovaEditor.exe" -ForegroundColor Yellow
