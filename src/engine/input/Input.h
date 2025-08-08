#pragma once
struct GLFWwindow;
namespace nova {
class Input {
public:
    static void Init(GLFWwindow* win);
    static bool KeyDown(int key);
    static bool KeyPressed(int key);
    static void NewFrame();
private:
    static bool s_keys[512], s_prev[512];
};
}
