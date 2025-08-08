param(
  [string]$RepoRoot = "."
)
$ErrorActionPreference = "Stop"
$build = Join-Path $RepoRoot "build"
if (-not (Test-Path $build)) {
  Write-Host "Build folder not found. Run scripts\reconfigure_clean.ps1 first." -ForegroundColor Yellow
  exit 1
}
cmake --build $build --config Debug
$exe = Join-Path $build "bin\Debug\NovaEditor.exe"
Write-Host "If succeeded, run: `"$exe`""
