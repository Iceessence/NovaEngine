Param(
  [string]$RepoRoot = "."
)
$ErrorActionPreference = "Stop"
$target = Join-Path $RepoRoot "src\engine\editor\Editor.cpp"
if (!(Test-Path $target)) {
  Write-Error "File not found: $target"
}
$orig = Get-Content -Raw $target

# If ui_frame_begun is already declared anywhere in the file, do nothing.
if ($orig -match '\bui_frame_begun\b') {
  Write-Host "ui_frame_begun already present. No changes made."
  exit 0
}

# Insert declaration just after the opening brace of Editor::Run()
$pattern = 'void\s+Editor::Run\s*\([^)]*\)\s*\{'
$match = [regex]::Match($orig, $pattern, 'IgnoreCase, Multiline')
if (!$match.Success) {
  Write-Error "Could not locate 'void Editor::Run()' function header."
}

$insertPos = $match.Index + $match.Length
$insertion = "`r`n    bool ui_frame_begun = false;  // inserted by apply_editor_ui_flag.ps1`r`n"

$updated = $orig.Substring(0, $insertPos) + $insertion + $orig.Substring($insertPos)

# Backup then write
Copy-Item -Path $target -Destination ($target + ".bak") -Force
Set-Content -Path $target -Value $updated -Encoding UTF8

Write-Host "Inserted 'bool ui_frame_begun = false;' after Editor::Run() brace."
