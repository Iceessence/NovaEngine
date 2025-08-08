// Patched Editor.cpp (guard UI against missing ImGui frame)
// Drop-in replacement for: src/engine/editor/Editor.cpp

#include "Editor.h"
#include "core/Log.h"
#include <imgui.h>

static bool __NovaImGuiReady()
{
    ImGuiContext* ctx = ImGui::GetCurrentContext();
    if (!ctx) return false;
    ImGuiIO& io = ImGui::GetIO();
    if (!(io.BackendRendererUserData && io.BackendPlatformUserData)) return false;
    if (!(io.Fonts && io.Fonts->IsBuilt())) return false;
    return true;
}

namespace nova {

// Provided by renderer (or shimmed) to indicate a frame has begun
extern bool g_ui_frame_begun;

void Editor::DrawUI()
{
    if (!g_ui_frame_begun || ImGui::GetCurrentContext() == nullptr)
        return;

    ImGuiID dock_id = ImGui::GetID("MainDock"); ImGui::DockSpace(dock_id, ImVec2(0,0), ImGuiDockNodeFlags_None);

    if (ImGui::Begin("Hierarchy"))
    {
        ImGui::TextUnformatted("Hello PBR Cube");
    }
    ImGui::End();

    if (ImGui::Begin("Console"))
    {
        ImGui::TextWrapped("Logs streaming here...");
    }
    ImGui::End();
}

} // namespace nova
ImGui::End();
}

void nova::Editor::DrawUI() {
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);
    ImGui::Render();
}

