#pragma once
#include "IRenderer.h"
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>

namespace nova {

class OpenGLRenderer : public IRenderer {
public:
    OpenGLRenderer() = default;
    ~OpenGLRenderer() = default;

    bool Init(void* windowHandle) override;
    void Shutdown() override;
    void BeginFrame() override;
    void DrawCube(const glm::mat4& model, const glm::vec3& baseColor, float metallic, float roughness) override;
    void EndFrame() override;
    RenderStats Stats() const override;

private:
    GLFWwindow* m_window = nullptr;
    bool m_initialized = false;
    
    // OpenGL objects
    unsigned int m_vertexBuffer = 0;
    unsigned int m_indexBuffer = 0;
    unsigned int m_vertexArray = 0;
    unsigned int m_shaderProgram = 0;
    
    // Camera matrices
    glm::mat4 m_projection;
    glm::mat4 m_view;
    
    // Stats
    RenderStats m_stats;
    
    // Helper functions
    bool CreateShaders();
    bool CreateBuffers();
    void SetupCamera();
};

} // namespace nova