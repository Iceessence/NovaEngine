param(
    [switch]$User,
    [switch]$Machine
)
# Remove NOVA_DISABLE_IMGUI from all scopes (Process + optional User/Machine)
[Environment]::SetEnvironmentVariable("NOVA_DISABLE_IMGUI", $null, "Process")
if ($User)   { [Environment]::SetEnvironmentVariable("NOVA_DISABLE_IMGUI", $null, "User") }
if ($Machine){ try { [Environment]::SetEnvironmentVariable("NOVA_DISABLE_IMGUI", $null, "Machine") } catch {} }
Write-Host "NOVA_DISABLE_IMGUI cleared (Process)" -ForegroundColor Green
if ($User)   { Write-Host "NOVA_DISABLE_IMGUI cleared (User)" -ForegroundColor Green }
if ($Machine){ Write-Host "NOVA_DISABLE_IMGUI cleared (Machine, may require admin)" -ForegroundColor Yellow }
