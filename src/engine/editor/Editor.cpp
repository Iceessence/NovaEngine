#include "Editor.h"
#include "core/Log.h"

#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>

namespace nova {

// Esc key = close helper
static void OnKey(GLFWwindow* w, int key, int scancode, int action, int mods) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) {
        glfwSetWindowShouldClose(w, GLFW_TRUE);
    }
}

void Editor::Init() {
    NOVA_INFO("Editor::Init");

    if (!glfwInit()) {
        NOVA_INFO("GLFW init failed");
        return;
    }

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API); // we're Vulkan
    if (!m_window) {
        m_window = glfwCreateWindow(1280, 720, "NovaEditor", nullptr, nullptr);
    }
    if (m_window) {
        glfwSetKeyCallback(m_window, OnKey);
        NOVA_INFO("GLFW window created (1280x720)");
    }
}

void Editor::Run() {
    NOVA_INFO("Editor::Run — entering loop");
    if (!m_window) {
        NOVA_INFO("No window; exiting run loop");
        return;
    }

    // Run until user closes the window (or presses Esc).
    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();
        // TODO: renderer BeginFrame/EndFrame and ImGui drawing go here
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS idle
    }

    NOVA_INFO("Editor::Run — leaving loop");
}

void Editor::Shutdown() {
    NOVA_INFO("Editor::Shutdown");
    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

void Editor::DrawUI() {
    // TODO: your ImGui panels
}

} // namespace nova
