param(
  [Parameter(Mandatory=$true)][string]$RepoRoot
)
$ErrorActionPreference = "Stop"
$build = Join-Path $RepoRoot "build"
if (Test-Path $build) { Remove-Item -Recurse -Force $build }
& cmake -S $RepoRoot -B $build -DCMAKE_BUILD_TYPE=Debug
if ($LASTEXITCODE -ne 0) { throw "Configure failed." }
& cmake --build $build --config Debug
if ($LASTEXITCODE -ne 0) { throw "Build failed." }
Write-Host "Run: `n  $build\bin\Debug\NovaEditor.exe"
