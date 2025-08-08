#pragma once
struct GLFWwindow;

namespace nova {

class Editor {
public:
    void Init();
    void Run();
    void Shutdown();
    void DrawUI();

private:
    GLFWwindow* m_window = nullptr; // <-- store the GLFW window here
};

} // namespace nova
