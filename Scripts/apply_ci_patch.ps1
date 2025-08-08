param([string]$RepoRoot = ".")

function Edit-Text($path, [ScriptBlock]$mutate) {
  $t = Get-Content -Raw -LiteralPath $path -Encoding UTF8
  $new = & $mutate $t
  if ($new -ne $t) {
    $bak = "$path.bak-$(Get-Date -Format yyyyMMdd-HHmmss)"
    Copy-Item -LiteralPath $path $bak -Force
    Set-Content -LiteralPath $path -Encoding UTF8 -NoNewline -Value $new
    Write-Host "Patched: $path"
    return $true
  } else {
    Write-Host "No changes: $path"
    return $false
  }
}

# 1) Ensure VK_CHECK exists in VulkanHelpers.h (CI was missing it)
$helpers = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanHelpers.h'
if (Test-Path $helpers) {
  Edit-Text $helpers {
    param($t)
    if ($t -notmatch '^\s*#\s*define\s+VK_CHECK\b') {
      $inject = @"
#ifndef VK_CHECK
#define VK_CHECK(expr) do { VkResult _vk__res = (expr); if (_vk__res != VK_SUCCESS) { /* log if you have a macro */ /*fprintf(stderr,"VK error %d at %s:%d\n", (int)_vk__res, __FILE__, __LINE__);*/ __debugbreak(); } } while(0)
#endif
"@
      # Put it near the top after includes/pragma once
      if ($t -match '(\#pragma once[^\r\n]*\r?\n)') {
        $t -replace '(\#pragma once[^\r\n]*\r?\n)', "`$1$inject`r`n"
      } else {
        $t + "`r`n$inject"
      }
    } else { $t }
  } | Out-Null
}

# 2) Make ImGui init call compatible with CI’s backend (1-arg Init)
$vr_cpp = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.cpp'
if (Test-Path $vr_cpp) {
  Edit-Text $vr_cpp {
    param($t)
    $t = $t -replace 'ImGui_ImplVulkan_Init\s*\(\s*&?(\w+)\s*,\s*(\w+)\s*\)', 'ImGui_ImplVulkan_Init(&$1)'  # 2 args -> 1
    # Remove duplicate free function impl of IsImGuiReady if present
    $t = [regex]::Replace($t, '(?s)\bbool\s+nova::VulkanRenderer::IsImGuiReady\s*\(\s*\)\s*const\s*\{.*?\}', {
      # keep only one – replace any free impl body with inline shim that returns member
      "/* removed duplicate IsImGuiReady body (handled inline in header) */"
    })
    $t
  } | Out-Null
}

# 3) Ensure header has inline IsImGuiReady and no stray tokens
$vr_h = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.h'
if (Test-Path $vr_h) {
  Edit-Text $vr_h {
    param($t)
    # Guarantee a single inline getter
    if ($t -notmatch 'bool\s+IsImGuiReady\s*\(\)\s*const') {
      $t = $t -replace '(class\s+VulkanRenderer\s*:[^{]+{)', "`$1`r`npublic:`r`n    bool IsImGuiReady() const { return m_imguiReady; }`r`n"
    }
    $t
  } | Out-Null
}

# 4) De-duplicate Editor::DrawUI and avoid DockSpaceOverViewport signature mismatch
$ed_cpp = Join-Path $RepoRoot 'src\engine\editor\Editor.cpp'
if (Test-Path $ed_cpp) {
  Edit-Text $ed_cpp {
    param($t)
    # Replace DockSpaceOverViewport(...) with a stable DockSpace(...) call
    $t = [regex]::Replace($t, 'ImGui::DockSpaceOverViewport\s*\(.*?\)\s*;', {
      'ImGuiID dock_id = ImGui::GetID("MainDock"); ImGui::DockSpace(dock_id, ImVec2(0,0), ImGuiDockNodeFlags_None);'
    }, 'Singleline')

    # Find ALL DrawUI definitions and keep only the first
    $rx = [regex]'void\s+nova::Editor::DrawUI\s*\(\s*\)\s*\{.*?\}'
    $m = $rx.Matches($t)
    if ($m.Count -gt 1) {
      # Build a new text keeping first, commenting others
      $pieces = @()
      $last = 0
      for ($i=0; $i -lt $m.Count; $i++) {
        $pieces += $t.Substring($last, $m[$i].Index - $last)
        if ($i -eq 0) {
          $pieces += $m[$i].Value
        } else {
          $pieces += "/* duplicate DrawUI removed by script */"
        }
        $last = $m[$i].Index + $m[$i].Length
      }
      $pieces += $t.Substring($last)
      $t = ($pieces -join '')
    }
    $t
  } | Out-Null
}

Write-Host "Done. Now build locally (optional), then commit & push to re-run CI:"