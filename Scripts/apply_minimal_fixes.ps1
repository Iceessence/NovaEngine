param([string]$RepoRoot='.')
# Minimal, surgical fixes for current build errors.
# Usage:
#   powershell -ExecutionPolicy Bypass -File scripts\apply_minimal_fixes.ps1 -RepoRoot .

$ErrorActionPreference = 'Stop'

function Get-Text { param($p) ; return (Get-Content $p -Raw) }
function Save-Text { param($p,$t) ; Set-Content -NoNewline -Path $p -Value $t }

function Ensure-Include {
  param($file, $header)
  $t = Get-Text $file
  if ($t -notmatch [regex]::Escape($header)) {
    # insert after first #include line if present, else at top
    if ($t -match '^\s*#include[^\n]*\n') {
      $t = $t.replace($matches[0], $matches[0] + "#include {0}`r`n" -f $header, 1)
    } else {
      $t = "#include {0}`r`n{1}" -f $header, $t
    }
    Save-Text $file $t
    Write-Host "Added include $header to $file"
  }
}

function Replace-Include {
  param($file, $from, $to)
  $t = Get-Text $file
  $new = $t -replace ('#include\s*"{0}"' -f [regex]::Escape($from)), ('#include "{0}"' -f $to)
  if ($new != $t) {
    Save-Text $file $new
    Write-Host "Rewrote include $from -> $to in $file"
  }
}

function Ensure-Within-Class {
  param(
    [string]$file,
    [string]$className,
    [string]$markerRegex,
    [string]$insertText
  )
  $t = Get-Text $file
  if ($t -match "class\s+$className[^{]*\{") {
    $start = $matches[0].Length + $matches[0].Index
    # Find the closing brace of the class (the one followed by ;)
    $closingIdx = $null
    # naive scan for '};' from the end to be robust
    $idx = $t.LastIndexOf("};")
    if ($idx -gt $start) { $closingIdx = $idx }
    if ($closingIdx -eq $null) { throw "Couldn't locate end of class $className in $file" }
    $classBody = $t.Substring(0, $closingIdx)
    if ($classBody -notmatch $markerRegex) {
      $patched = $t.Substring(0, $closingIdx) + "`r`n" + $insertText + "`r`n" + $t.Substring($closingIdx)
      Save-Text $file $patched
      Write-Host "Injected into class $className in $file"
    }
  } else {
    throw "Couldn't find class $className in $file"
  }
}

# Paths
$vrh = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.h'
$vrc = Join-Path $RepoRoot 'src\engine\renderer\vk\VulkanRenderer.cpp'
$edh = Join-Path $RepoRoot 'src\engine\editor\Editor.h'
$edc = Join-Path $RepoRoot 'src\engine\editor\Editor.cpp'

# 1) Fix includes for Log.h (as you said it's at src\engine\core\Log.h)
foreach ($f in @($vrc, $edc)) {
  if (Test-Path $f) {
    Replace-Include -file $f -from 'Log.h' -to 'core/Log.h'
    Ensure-Include -file $f -header '<algorithm>'
  }
}

# 2) Define VK_CHECK if it's missing (in .cpp)
if (Test-Path $vrc) {
  $t = Get-Text $vrc
  if ($t -notmatch 'VK_CHECK') {
    $t = $t + @'
#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _vk_res = (x); if (_vk_res != VK_SUCCESS) { fprintf(stderr,"Vulkan error %d at %s:%d\n",(int)_vk_res,__FILE__,__LINE__); __debugbreak(); } } while(0)
#endif
'@
    Save-Text $vrc $t
    Write-Host "Added VK_CHECK macro to $vrc"
  }
}

# 3) Ensure methods/members exist INSIDE class VulkanRenderer
if (Test-Path $vrh) {
  $methodBlock = @'
public:
    void InitImGui();
    void BeginFrame();
    void EndFrame(VkCommandBuffer cmd);
    bool IsImGuiReady() const { return m_imguiReady; }
private:
    bool                    m_imguiReady = false;
    VkDescriptorPool        m_imguiPool = VK_NULL_HANDLE;
'@
  Ensure-Within-Class -file $vrh -className 'VulkanRenderer' -markerRegex '\bm_imguiReady\b' -insertText $methodBlock
}

# 4) Editor.h: declare DrawUI()
if (Test-Path $edh) {
  Ensure-Within-Class -file $edh -className 'Editor' -markerRegex '\bDrawUI\s*\(' -insertText 'public: void DrawUI();'
}

# 5) Editor.cpp: add DrawUI impl (if absent) and fix DockSpaceOverViewport call
if (Test-Path $edc) {
  $t = Get-Text $edc
  # fix DockSpaceOverViewport misuse
  $t = $t -replace 'DockSpaceOverViewport\s*\(\s*[^,)\r\n]*\s*,\s*([^,\r\n)]+)\s*\)', 'DockSpaceOverViewport(ImGui::GetMainViewport(), $1)'
  if ($t -notmatch '(?ms)void\s+nova::Editor::DrawUI\s*\(') {
    $t += @'

void nova::Editor::DrawUI() {
    if (ImGui::GetCurrentContext() == nullptr) return;
    ImGui::DockSpaceOverViewport(ImGui::GetMainViewport(), ImGuiDockNodeFlags_PassthruCentralNode);
    if (ImGui::Begin("Nova")) {
        ImGui::TextUnformatted("Hello from NovaEditor");
    }
    ImGui::End();
}
'
    Save-Text $edc $t
    Write-Host "Added Editor::DrawUI() to $edc"
  } else {
    if ($t -ne (Get-Text $edc)) { Save-Text $edc $t ; Write-Host "Patched DockSpaceOverViewport in $edc" }
  }
}

Write-Host "All minimal fixes applied. Now run: cmake --build build --config Debug"
