param(
  [string]$RepoRoot = ".",
  [string]$Generator = "Visual Studio 17 2022",
  [string]$Arch = "x64"
)

$ErrorActionPreference = "Stop"

$build = Join-Path $RepoRoot "build"
if (Test-Path $build) {
  Write-Host "Removing existing build folder: $build"
  Remove-Item $build -Recurse -Force
}

Write-Host "Configuring CMake..."
cmake -S $RepoRoot -B $build -G $Generator -A $Arch

Write-Host "Building Debug..."
cmake --build $build --config Debug
