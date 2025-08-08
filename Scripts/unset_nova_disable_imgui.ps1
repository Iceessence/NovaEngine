# Clears NOVA_DISABLE_IMGUI in this PowerShell session
Remove-Item Env:NOVA_DISABLE_IMGUI -ErrorAction SilentlyContinue
Write-Host "NOVA_DISABLE_IMGUI cleared for this session. To make it permanent, remove it from your user/system env vars."
