NovaEngine Vulkan/ImGui hotfix pack (v7)

Usage (PowerShell):
1) cd C:\Dev\NovaEngine
2) Expand-Archive -Path <downloaded zip> -DestinationPath . -Force
3) powershell -ExecutionPolicy Bypass -File .\scripts\apply_vulkan_imgui_fix.ps1 -RepoRoot .
4) powershell -ExecutionPolicy Bypass -File .\scripts\reconfigure_clean.ps1 -RepoRoot .
5) .\build\bin\Debug\NovaEditor.exe

What it does:
- Fixes VulkanRenderer.h method signatures (InitImGui(GLFWwindow*), Begin/EndFrame) and removes duplicate inline IsImGuiReady body.
- Ensures m_imguiReady exists.
- Inserts volk/algorithm/VulkanHelpers includes.
- Normalizes member access and fixes DockSpaceOverViewport param order in Editor.cpp.
- Adds VK_NO_PROTOTYPES to CMakeLists.
