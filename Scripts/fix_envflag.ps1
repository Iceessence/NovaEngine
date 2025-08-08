param(
    [string]$RepoRoot = "."
)

$ErrorActionPreference = "Stop"

$vr = Join-Path $RepoRoot "src\engine\renderer\vk\VulkanRenderer.cpp"
if (!(Test-Path $vr)) {
    Write-Error "VulkanRenderer.cpp not found at $vr"
    exit 1
}

$content = Get-Content -LiteralPath $vr -Raw
$orig = $content

# 1) Ensure headers needed for parsing env var safely (okay to prepend duplicates)
$std_headers = "#include <cstdlib>`r`n#include <string>`r`n#include <algorithm>`r`n"
if ($content -notmatch '(?m)^\s*#\s*include\s*<cstdlib>') {
    $content = $std_headers + $content
}

# 2) Insert forward declaration if missing
if ($content -notmatch 'namespace\s+nova\s*\{\s*bool\s+ShouldDisableImGui\s*\(\s*\)\s*;') {
    $fwd = "namespace nova { bool ShouldDisableImGui(); }`r`n"
    $content = $fwd + $content
}

# 3) Replace any naive checks like if(getenv("NOVA_DISABLE_IMGUI"))
$pattern = 'if\s*\(\s*(?:std::)?getenv\(\s*"NOVA_DISABLE_IMGUI"\s*\)\s*\)'
$replacement = 'if (nova::ShouldDisableImGui())'
$content = [regex]::Replace($content, $pattern, $replacement)

# 4) Append definition if not present
if ($content -notmatch '(?s)bool\s+ShouldDisableImGui\s*\(\s*\)\s*\{') {
$impl = @"
namespace nova {

bool ShouldDisableImGui()
{
    const char* e = std::getenv("NOVA_DISABLE_IMGUI");
    if (!e || !*e) return false;
    std::string v(e);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    auto ltrim = [](std::string& s){ s.erase(0, s.find_first_not_of(" \t\r\n")); };
    auto rtrim = [](std::string& s){ s.erase(s.find_last_not_of(" \t\r\n") + 1); };
    ltrim(v); rtrim(v);
    return (v=="1" || v=="true" || v=="yes" || v=="on");
}

} // namespace nova

"@
    $content = $content + "`r`n" + $impl
}

if ($content -ne $orig) {
    Set-Content -LiteralPath $vr -Value $content -NoNewline
    Write-Host "Patched NOVA_DISABLE_IMGUI handling in $vr"
} else {
    Write-Host "No changes made (pattern not found) in $vr"
}
