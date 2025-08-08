#include "Editor.h"
#include "imgui.h"
#include "imgui_internal.h"

void nova::Editor::DrawUI() {
    ImGui::NewFrame();
    ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);
    ImGui::Render();
}
