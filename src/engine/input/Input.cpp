#include "Input.h"
#include <GLFW/glfw3.h>
namespace nova {
bool Input::s_keys[512]{}, Input::s_prev[512]{};
void Input::Init(GLFWwindow* w){ (void)w; }
bool Input::KeyDown(int key){ return key<512 ? s_keys[key] : false; }
bool Input::KeyPressed(int key){ return key<512 ? (s_keys[key] && !s_prev[key]) : false; }
void Input::NewFrame(){
    for(int i=0;i<512;i++){ s_prev[i]=s_keys[i]; s_keys[i]=false; }
}
}
