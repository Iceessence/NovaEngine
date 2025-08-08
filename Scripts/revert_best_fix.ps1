param(
    [string]$RepoRoot = "."
)

$ErrorActionPreference = "Stop"

$repo = (Resolve-Path $RepoRoot).Path
$cmake = Join-Path $repo "CMakeLists.txt"

if (!(Test-Path $cmake)) {
    Write-Error "CMakeLists.txt not found at: $cmake"
}

# Find the latest backup
$backups = Get-ChildItem -LiteralPath $repo -Filter "CMakeLists.txt.bak-*" | Sort-Object LastWriteTime -Descending
if (!$backups -or $backups.Count -eq 0) {
    Write-Error "No backups found (CMakeLists.txt.bak-*) to restore."
}

$latest = $backups[0].FullName
Copy-Item -LiteralPath $latest -Destination $cmake -Force
Write-Host "Restored $cmake from $latest"
