#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>

// Simple vertex shader
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 Normal;

void main()
{
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

// Simple fragment shader
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 Normal;

uniform vec3 uColor;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(vec3(0.4, 1.0, 0.2));
    float NoL = max(dot(N, L), 0.0);
    vec3 color = uColor * NoL;
    FragColor = vec4(color, 1.0);
}
)";

// Cube vertex data
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
};

static const std::vector<Vertex> cubeVertices = {
    // Front face
    {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}},
    
    // Back face
    {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}},
    
    // Left face
    {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}},
    
    // Right face
    {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}},
    
    // Top face
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}},
    
    // Bottom face
    {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
    {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}},
};

static const std::vector<unsigned int> cubeIndices = {
    0,  1,  2,    2,  3,  0,   // front
    4,  5,  6,    6,  7,  4,   // back
    8,  9,  10,   10, 11, 8,   // left
    12, 13, 14,   14, 15, 12,  // right
    16, 17, 18,   18, 19, 16,  // top
    20, 21, 22,   22, 23, 20   // bottom
};

// GLAD function
typedef void (*GLADloadproc)(const char* name);
int gladLoadGLLoader(GLADloadproc load) {
    // Simple implementation - just return success
    return 1;
}

// OpenGL function declarations (simplified)
extern "C" {
    unsigned int glCreateShader(unsigned int type);
    void glShaderSource(unsigned int shader, int count, const char* const* string, const int* length);
    void glCompileShader(unsigned int shader);
    void glGetShaderiv(unsigned int shader, unsigned int pname, int* params);
    void glGetShaderInfoLog(unsigned int shader, int bufSize, int* length, char* infoLog);
    unsigned int glCreateProgram(void);
    void glAttachShader(unsigned int program, unsigned int shader);
    void glLinkProgram(unsigned int program);
    void glGetProgramiv(unsigned int program, unsigned int pname, int* params);
    void glGetProgramInfoLog(unsigned int program, int bufSize, int* length, char* infoLog);
    void glDeleteShader(unsigned int shader);
    void glDeleteProgram(unsigned int program);
    void glGenVertexArrays(int n, unsigned int* arrays);
    void glBindVertexArray(unsigned int array);
    void glGenBuffers(int n, unsigned int* buffers);
    void glBindBuffer(unsigned int target, unsigned int buffer);
    void glBufferData(unsigned int target, long long size, const void* data, unsigned int usage);
    void glVertexAttribPointer(unsigned int index, int size, unsigned int type, unsigned char normalized, int stride, const void* pointer);
    void glEnableVertexAttribArray(unsigned int index);
    void glUseProgram(unsigned int program);
    void glUniformMatrix4fv(int location, int count, unsigned char transpose, const float* value);
    void glUniform3fv(int location, int count, const float* value);
    int glGetUniformLocation(unsigned int program, const char* name);
    void glDrawElements(unsigned int mode, int count, unsigned int type, const void* indices);
    void glClearColor(float red, float green, float blue, float alpha);
    void glClear(unsigned int mask);
    void glEnable(unsigned int cap);
    void glDepthFunc(unsigned int func);
    void glDeleteBuffers(int n, const unsigned int* buffers);
    void glDeleteVertexArrays(int n, const unsigned int* arrays);
}

// OpenGL constants
#define GL_VERTEX_SHADER 0x8B31
#define GL_FRAGMENT_SHADER 0x8B30
#define GL_COMPILE_STATUS 0x8B81
#define GL_LINK_STATUS 0x8B82
#define GL_ARRAY_BUFFER 0x8892
#define GL_ELEMENT_ARRAY_BUFFER 0x8893
#define GL_STATIC_DRAW 0x88E4
#define GL_TRIANGLES 0x0004
#define GL_UNSIGNED_INT 0x1405
#define GL_DEPTH_TEST 0x0B71
#define GL_LESS 0x0201
#define GL_COLOR_BUFFER_BIT 0x00004000
#define GL_DEPTH_BUFFER_BIT 0x00000100
#define GL_FLOAT 0x1406
#define GL_FALSE 0

int main() {
    std::cout << "Starting Nova Engine - Spinning Cube Demo" << std::endl;
    
    // Initialize GLFW
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }
    
    // Create window
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    
    GLFWwindow* window = glfwCreateWindow(1280, 720, "Nova Engine - Spinning Cube", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    
    glfwMakeContextCurrent(window);
    
    // Initialize OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return -1;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Create shaders
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cerr << "Vertex shader compilation failed: " << infoLog << std::endl;
        return -1;
    }
    
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cerr << "Fragment shader compilation failed: " << infoLog << std::endl;
        return -1;
    }
    
    // Create shader program
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cerr << "Shader program linking failed: " << infoLog << std::endl;
        return -1;
    }
    
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    // Create vertex array and buffers
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    glBindVertexArray(VAO);
    
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(Vertex), cubeVertices.data(), GL_STATIC_DRAW);
    
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), cubeIndices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    // Setup camera matrices
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    // Get uniform locations
    int mvpLocation = glGetUniformLocation(shaderProgram, "uMVP");
    int modelLocation = glGetUniformLocation(shaderProgram, "uModel");
    int colorLocation = glGetUniformLocation(shaderProgram, "uColor");
    
    std::cout << "Rendering loop starting..." << std::endl;
    
    // Render loop
    double angle = 0.0;
    auto lastTime = glfwGetTime();
    
    while (!glfwWindowShouldClose(window)) {
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        
        // Update rotation
        angle += 60.0 * deltaTime; // 60 degrees per second
        if (angle > 360.0) angle -= 360.0;
        
        // Clear the screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Use shader program
        glUseProgram(shaderProgram);
        
        // Create model matrix for spinning cube
        glm::mat4 model = glm::mat4(1.0f);
        model = glm::rotate(model, glm::radians(static_cast<float>(angle)), glm::vec3(0.0f, 1.0f, 0.0f));
        model = glm::rotate(model, glm::radians(static_cast<float>(angle * 0.5f)), glm::vec3(1.0f, 0.0f, 0.0f));
        
        // Set uniforms
        glm::mat4 mvp = projection * view * model;
        glUniformMatrix4fv(mvpLocation, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(modelLocation, 1, GL_FALSE, glm::value_ptr(model));
        glUniform3fv(colorLocation, 1, glm::value_ptr(glm::vec3(0.8f, 0.3f, 0.2f)));
        
        // Draw the cube
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, static_cast<int>(cubeIndices.size()), GL_UNSIGNED_INT, 0);
        
        // Swap buffers and poll events
        glfwSwapBuffers(window);
        glfwPollEvents();
    }
    
    // Cleanup
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    glDeleteProgram(shaderProgram);
    
    glfwDestroyWindow(window);
    glfwTerminate();
    
    std::cout << "Nova Engine shutdown complete" << std::endl;
    return 0;
}