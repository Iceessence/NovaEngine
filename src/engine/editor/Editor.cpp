#include "Editor.h"
#include "engine/core/Log.h"

#include <thread>
#include <chrono>

// If you have ImGui available, we'll render a dockspace when a context exists.
#include <imgui.h>

namespace nova {

void Editor::Init() {
    NOVA_INFO("Editor::Init");
}

void Editor::Run() {
    NOVA_INFO("Editor::Run — stub loop (10s)");
    auto end = std::chrono::steady_clock::now() + std::chrono::seconds(10);
    while (std::chrono::steady_clock::now() < end) {
        // Do per-frame work here later (poll events, render, etc.)
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
        // Optionally draw UI each tick (safe even if ImGui isn't ready)
        DrawUI();
    }
    NOVA_INFO("Editor::Run — done");
}

void Editor::Shutdown() {
    NOVA_INFO("Editor::Shutdown");
}

void Editor::DrawUI() {
    // Only call ImGui if a context exists (prevents crashes if not initialized yet)
    if (ImGui::GetCurrentContext()) {
        ImGui::NewFrame();
        ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);
        ImGui::Render();
    }
    // else: no-op
}

} // namespace nova
