#pragma once
#include "engine/renderer/OpenGLRenderer.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

struct GLFWwindow;

namespace nova {

class Editor {
public:
    void Init();
    void Run();
    void Shutdown();
    void DrawUI();

private:
    GLFWwindow* m_window = nullptr;
    OpenGLRenderer m_renderer;
    
    // Camera and rendering state
    glm::mat4 m_projection;
    glm::mat4 m_view;
    float m_cameraDistance = 3.0f;
    float m_cameraRotationX = 0.0f;
    float m_cameraRotationY = 0.0f;
};

} // namespace nova
