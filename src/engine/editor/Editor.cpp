#include "Editor.h"
#include "engine/core/Log.h"
#include <GLFW/glfw3.h>
#include <thread>
#include <chrono>

namespace nova {

void Editor::Init() {
    NOVA_INFO("Editor::Init");
    if (!glfwInit()) {
        NOVA_INFO("GLFW init failed");
        return;
    }
    
    // Create window with OpenGL support
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    m_window = glfwCreateWindow(1280, 720, "NovaEditor", nullptr, nullptr);
    if (!m_window) {
        NOVA_INFO("GLFW create window failed");
        return;
    }
    NOVA_INFO("GLFW window created (1280x720)");
    
    // Initialize OpenGL renderer
    if (!m_renderer.Init(m_window)) {
        NOVA_ERROR("Failed to initialize OpenGL renderer");
        return;
    }
    
    // Setup camera matrices
    m_projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    m_view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, m_cameraDistance),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

void Editor::Run() {
    NOVA_INFO("Editor::Run — entering loop");

    double angle = 0.0;
    auto last = std::chrono::steady_clock::now();

    while (m_window && !glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        // Calculate delta time
        auto now = std::chrono::steady_clock::now();
        double dt = std::chrono::duration<double>(now - last).count();
        last = now;
        
        // Update rotation
        angle += 60.0 * dt; // 60 degrees per second
        if (angle > 360.0) angle -= 360.0;
        
        // Update camera
        m_cameraRotationY += 30.0 * dt; // 30 degrees per second
        if (m_cameraRotationY > 360.0) m_cameraRotationY -= 360.0;
        
        // Update view matrix
        glm::mat4 cameraRotation = glm::rotate(glm::mat4(1.0f), glm::radians(m_cameraRotationY), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::vec3 cameraPos = glm::vec3(cameraRotation * glm::vec4(0.0f, 0.0f, m_cameraDistance, 1.0f));
        m_view = glm::lookAt(cameraPos, glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

        // Begin rendering
        m_renderer.BeginFrame();
        
        // Create model matrix for spinning cube
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(static_cast<float>(angle)), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(static_cast<float>(angle * 0.5f)), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Draw the cube
        m_renderer.DrawCube(model, glm::vec3(0.8f, 0.3f, 0.2f), 0.5f, 0.3f);
        
        // End rendering
        m_renderer.EndFrame();
        
        // Small delay to maintain frame rate
        std::this_thread::sleep_for(std::chrono::milliseconds(16));
    }

    NOVA_INFO("Editor::Run — leaving loop");
}

void Editor::Shutdown() {
    NOVA_INFO("Editor::Shutdown");
    m_renderer.Shutdown();
    if (m_window) { 
        glfwDestroyWindow(m_window); 
        m_window = nullptr; 
    }
    glfwTerminate();
}

void Editor::DrawUI() {
    // UI drawing will be implemented later
}

} // namespace nova
