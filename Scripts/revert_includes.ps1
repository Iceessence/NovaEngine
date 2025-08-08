param(
    [Parameter(Mandatory=$true)][string]$RepoRoot
)
$ErrorActionPreference = "Stop"

$targets = @(
    (Join-Path $RepoRoot "src\engine\editor\Editor.cpp"),
    (Join-Path $RepoRoot "src\engine\renderer\vk\VulkanRenderer.cpp")
)

foreach ($f in $targets) {
    if (Test-Path $f) {
        $orig = Get-Content $f -Raw
        $new  = $orig -replace '(["<])core/Log\.h([">])','$1Log.h$2'
        if ($new -ne $orig) {
            Set-Content -Path $f -Value $new -Encoding UTF8
            Write-Host "Reverted include in $f"
        } else {
            Write-Host "No change needed in $f"
        }
    } else {
        Write-Host "Missing file: $f"
    }
}
Write-Host "Include revert completed."
