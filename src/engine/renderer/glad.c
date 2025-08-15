#include <glad/glad.h>
#include <GLFW/glfw3.h>

// Simple GLAD implementation
int gladLoadGLLoader(GLADloadproc load) {
    // For now, we'll just return success
    // In a real implementation, this would load OpenGL function pointers
    return 1;
}