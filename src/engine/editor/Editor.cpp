#include "Editor.h"
#include "engine/core/Log.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <thread>
#include <chrono>

namespace nova {

void Editor::Init() {
    NOVA_INFO("Editor::Init");
    if (!glfwInit()) {
        NOVA_INFO("GLFW init failed");
        return;
    }
    m_window = glfwCreateWindow(1280, 720, "NovaEditor", nullptr, nullptr);
    if (!m_window) {
        NOVA_INFO("GLFW create window failed");
        return;
    }
    NOVA_INFO("GLFW window created (1280x720)");
}

void Editor::Run() {
    NOVA_INFO("Editor::Run — entering loop");

    double angle = 0.0;
    auto last = std::chrono::steady_clock::now();

    while (m_window && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // Only use ImGui if a context exists (prevents crashes)
        if (ImGui::GetCurrentContext()) {
            ImGui::NewFrame();
            ImGui::DockSpaceOverViewport(0, ImGui::GetMainViewport(), ImGuiDockNodeFlags_None, nullptr);

            ImGui::Begin("Nova Panel");
            auto now = std::chrono::steady_clock::now();
            double dt = std::chrono::duration<double>(now - last).count();
            last = now;
            angle += 60.0 * dt;
            if (angle > 360.0) angle -= 360.0;
            ImGui::Text("Angle: %.2f deg", angle);
            ImGui::End();

            ImGui::Render();
        } else {
            // No ImGui yet — keep the window responsive
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }

    NOVA_INFO("Editor::Run — leaving loop");
}

void Editor::Shutdown() {
    NOVA_INFO("Editor::Shutdown");
    if (m_window) { glfwDestroyWindow(m_window); m_window = nullptr; }
    glfwTerminate();
}

} // namespace nova
