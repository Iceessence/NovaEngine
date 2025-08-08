param(
  [string]$RepoRoot = "."
)

$ErrorActionPreference = "Stop"

$cm = Join-Path $RepoRoot "CMakeLists.txt"
if (!(Test-Path $cm)) {
  Write-Error "CMakeLists.txt not found at $cm"
  exit 1
}

# Backup
$timestamp = Get-Date -Format "yyyyMMdd-HHmmss"
$backup = "$cm.bak-$timestamp"
Copy-Item $cm $backup -Force

# Load raw and remove any stray backtick characters (0x60) that may have been injected (e.g., a literal `n).
$raw = Get-Content $cm -Raw
$backtick = [string][char]0x60
$clean = $raw -replace [regex]::Escape($backtick), ""

# Strip potential UTF-8 BOM
if ($clean.Length -gt 0 -and [int]$clean[0] -eq 0xFEFF) {
  $clean = $clean.Substring(1)
}

Set-Content -LiteralPath $cm -Value $clean -Encoding UTF8

Write-Host "Cleaned $cm . Backup at $backup"
Write-Host ""
Write-Host "Preview of first 20 lines:"
$i = 0
Get-Content $cm | Select-Object -First 20 | ForEach-Object {
  $script:i++ | Out-Null
  "{0,3}: {1}" -f $i, $_
} | Write-Host
