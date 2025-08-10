#include "engine/editor/Editor.h"
#include "imgui.h"
#include <chrono>
#include <thread>

#ifndef NOVA_INFO
#define NOVA_INFO(...) do{}while(0)
#endif

namespace nova {

static void DrawOverlay()
{
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);

    // Bright green crosshair so we KNOW overlay draws
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImVec2 c = ImGui::GetMainViewport()->GetCenter();
    dl->AddLine(ImVec2(c.x-50, c.y), ImVec2(c.x+50, c.y), IM_COL32(0,255,0,255), 3.0f);
    dl->AddLine(ImVec2(c.x, c.y-50), ImVec2(c.x, c.y+50), IM_COL32(0,255,0,255), 3.0f);

    ImGui::Begin("UI Alive", nullptr, ImGuiWindowFlags_AlwaysAutoResize);
    ImGui::Text("If you see this, ImGui is rendering.");
    ImGui::End();
}

void Editor::Init() {
    NOVA_INFO("Editor::Init");
}

void Editor::Run() {
    NOVA_INFO("Editor::Run - entering loop");

    using clock = std::chrono::steady_clock;
    auto end = clock::now() + std::chrono::seconds(15);

    while (clock::now() < end) {
        // Let the renderer tick backends in BeginFrame().
        ImGui::NewFrame();
        DrawOverlay();
        ImGui::Render();

        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    NOVA_INFO("Editor::Run - leaving loop");
}

void Editor::Shutdown() {
    NOVA_INFO("Editor::Shutdown");
}

} // namespace nova
