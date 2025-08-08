ImGui Enable Fix Bundle
=========================

What this fixes
---------------
- Your run logs show: "Skipping ImGui init via NOVA_DISABLE_IMGUI" even after you tried to clear the env var.
- Then ImGui asserts (`WithinFrameScope`) because the editor still draws UI while the renderer never started an ImGui frame.

What's inside
-------------
- **src/engine/renderer/vk/VulkanRenderer.cpp**: robust runtime check for `NOVA_DISABLE_IMGUI` (empty/0/false means NOT disabled), initializes ImGui, and begins/ends ImGui frames each frame. It also sets a global `g_ui_frame_begun` flag.
- **src/engine/editor/Editor.cpp**: guards UI drawing with `g_ui_frame_begun` and `ImGui::GetCurrentContext()` so it won't call ImGui when a frame hasn't started.
- **scripts/clear_nova_disable_imgui_permanently.ps1**: removes the env var from Process/User/Machine scopes.

How to apply
------------
1) **Back up** your originals (optional):
   - `copy src\engine\renderer\vk\VulkanRenderer.cpp src\engine\renderer\vk\VulkanRenderer.cpp.bak`
   - `copy src\engine\editor\Editor.cpp src\engine\editor\Editor.cpp.bak`

2) **Extract** these two files into your repo at the same paths:
   - `src\engine\renderer\vk\VulkanRenderer.cpp`
   - `src\engine\editor\Editor.cpp`

3) **(Strongly recommended) Nuke the env var** from your process and user/machine scopes:
   - Open PowerShell in the repo root and run:
     ```powershell
     powershell -ExecutionPolicy Bypass -File scripts\clear_nova_disable_imgui_permanently.ps1 -User -Machine
     ```
   - Close this PowerShell and open a **new** one (so the cleared env is not inherited).

4) **Rebuild & run**:
   ```powershell
   cmake --build build --config Debug
   .\build\bin\Debug\NovaEditor.exe
   ```

If the window opens and you still don't see ImGui panels, please grab `.logs\editor.log` and we’ll adjust.  
This patch doesn’t touch your Vulkan swapchain or PBR pipeline; it only normalizes ImGui bring-up and UI frame ordering.
