param(
    [Parameter(Mandatory=$true)][string]$RepoRoot,
    [string]$Config = "Debug"
)
$ErrorActionPreference = "Stop"

Push-Location $RepoRoot
try {
    # Ensure env flag is not set for this shell
    Remove-Item Env:NOVA_DISABLE_IMGUI -ErrorAction SilentlyContinue

    # Revert includes
    & powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "revert_includes.ps1") -RepoRoot $RepoRoot

    # Build
    cmake --build build --config $Config

    # Run (if build produced the exe)
    $exe = Join-Path $RepoRoot ("build\bin\{0}\NovaEditor.exe" -f $Config)
    if (Test-Path $exe) {
        Write-Host "Launching: $exe"
        & $exe
    } else {
        Write-Host "Executable not found at $exe"
    }
}
finally {
    Pop-Location
}
