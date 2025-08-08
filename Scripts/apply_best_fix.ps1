param(
    [string]$RepoRoot = "."
)

$ErrorActionPreference = "Stop"

# Resolve paths
$repo = (Resolve-Path $RepoRoot).Path
$cmake = Join-Path $repo "CMakeLists.txt"

if (!(Test-Path $cmake)) {
    Write-Error "CMakeLists.txt not found at: $cmake"
}

# Read CMakeLists
$text = Get-Content -Raw -LiteralPath $cmake

# Check if there's already a target_include_directories for NovaEngine that mentions src/engine
$alreadyHas = $false
if ($text -match 'target_include_directories\s*\(\s*NovaEngine\b[\s\S]*?src/engine') {
    $alreadyHas = $true
}

if (-not $alreadyHas) {
    $stamp = Get-Date -Format "yyyyMMddHHmmss"
    $backup = "$cmake.bak-$stamp"
    Copy-Item -LiteralPath $cmake -Destination $backup -Force
    Write-Host "Backed up CMakeLists.txt -> $backup"

    $block = @"
# Added by apply_best_fix.ps1 ($stamp)
target_include_directories(NovaEngine
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/engine
)
"@

    Add-Content -LiteralPath $cmake -Value "`r`n$block"
    Write-Host "Appended target_include_directories(NovaEngine ... src/engine) to CMakeLists.txt"
} else {
    Write-Host "CMakeLists already sets src/engine include dir for NovaEngine. No changes made."
}

# Clear NOVA_DISABLE_IMGUI in *this* shell to avoid font/newframe asserts
if (Test-Path Env:NOVA_DISABLE_IMGUI) {
    Remove-Item Env:NOVA_DISABLE_IMGUI -ErrorAction SilentlyContinue
    Write-Host "Cleared NOVA_DISABLE_IMGUI in this PowerShell session."
} else {
    Write-Host "NOVA_DISABLE_IMGUI not set in this session."
}

Write-Host ""
Write-Host "Next steps:"
Write-Host "  cmake -S $repo -B $($repo)\build -G 'Visual Studio 17 2022' -A x64"
Write-Host "  cmake --build $($repo)\build --config Debug"
Write-Host "  $($repo)\build\bin\Debug\NovaEditor.exe"
