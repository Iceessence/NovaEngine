#include "OpenGLRenderer.h"
#include "engine/core/Log.h"
#include "glad.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

namespace nova {

// Cube vertex data
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

static const std::vector<Vertex> cubeVertices = {
    // Front face
    {{-0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  0.0f,  1.0f}, {0.0f, 1.0f}},
    
    // Back face
    {{-0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {0.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  0.0f, -1.0f}, {1.0f, 1.0f}},
    
    // Left face
    {{-0.5f, -0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
    {{-0.5f, -0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
    {{-0.5f,  0.5f,  0.5f}, {-1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f, -0.5f}, {-1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
    
    // Right face
    {{ 0.5f, -0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 1.0f,  0.0f,  0.0f}, {0.0f, 1.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 1.0f,  0.0f,  0.0f}, {1.0f, 1.0f}},
    
    // Top face
    {{-0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 0.0f}},
    {{ 0.5f,  0.5f, -0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 0.0f}},
    {{ 0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {1.0f, 1.0f}},
    {{-0.5f,  0.5f,  0.5f}, { 0.0f,  1.0f,  0.0f}, {0.0f, 1.0f}},
    
    // Bottom face
    {{-0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 0.0f}},
    {{ 0.5f, -0.5f, -0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 0.0f}},
    {{ 0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {0.0f, 1.0f}},
    {{-0.5f, -0.5f,  0.5f}, { 0.0f, -1.0f,  0.0f}, {1.0f, 1.0f}},
};

static const std::vector<unsigned int> cubeIndices = {
    0,  1,  2,    2,  3,  0,   // front
    4,  5,  6,    6,  7,  4,   // back
    8,  9,  10,   10, 11, 8,   // left
    12, 13, 14,   14, 15, 12,  // right
    16, 17, 18,   18, 19, 16,  // top
    20, 21, 22,   22, 23, 20   // bottom
};

// Simple vertex shader
const char* vertexShaderSource = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;

uniform mat4 uMVP;
uniform mat4 uModel;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

void main()
{
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)";

// Simple fragment shader
const char* fragmentShaderSource = R"(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;

void main()
{
    vec3 N = normalize(Normal);
    vec3 L = normalize(vec3(0.4, 1.0, 0.2));
    vec3 V = normalize(vec3(0.0, 0.0, 1.0));
    
    float NoL = max(dot(N, L), 0.0);
    vec3 diffuse = uBaseColor * NoL;
    
    // Simple specular
    vec3 H = normalize(L + V);
    float NoH = max(dot(N, H), 0.0);
    float spec = pow(NoH, mix(8.0, 64.0, 1.0 - uRoughness));
    vec3 color = diffuse + spec * 0.15;
    
    FragColor = vec4(color, 1.0);
}
)";

bool OpenGLRenderer::Init(void* windowHandle) {
    if (m_initialized) return true;
    
    m_window = static_cast<GLFWwindow*>(windowHandle);
    if (!m_window) {
        NOVA_ERROR("Invalid window handle");
        return false;
    }
    
    NOVA_INFO("Initializing OpenGL renderer...");
    
    // Make the window's context current
    glfwMakeContextCurrent(m_window);
    
    // Initialize OpenGL
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        NOVA_ERROR("Failed to initialize GLAD");
        return false;
    }
    
    // Enable depth testing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    
    // Create shaders and buffers
    if (!CreateShaders()) {
        NOVA_ERROR("Failed to create shaders");
        return false;
    }
    
    if (!CreateBuffers()) {
        NOVA_ERROR("Failed to create buffers");
        return false;
    }
    
    SetupCamera();
    
    m_initialized = true;
    NOVA_INFO("OpenGL renderer initialized successfully");
    return true;
}

void OpenGLRenderer::Shutdown() {
    if (!m_initialized) return;
    
    if (m_vertexBuffer) glDeleteBuffers(1, &m_vertexBuffer);
    if (m_indexBuffer) glDeleteBuffers(1, &m_indexBuffer);
    if (m_vertexArray) glDeleteVertexArrays(1, &m_vertexArray);
    if (m_shaderProgram) glDeleteProgram(m_shaderProgram);
    
    m_initialized = false;
}

void OpenGLRenderer::BeginFrame() {
    if (!m_initialized) return;
    
    // Clear the screen
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    // Use shader program
    glUseProgram(m_shaderProgram);
}

void OpenGLRenderer::DrawCube(const glm::mat4& model, const glm::vec3& baseColor, float metallic, float roughness) {
    if (!m_initialized) return;
    
    // Bind vertex array
    glBindVertexArray(m_vertexArray);
    
    // Set uniforms
    glm::mat4 mvp = m_projection * m_view * model;
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uMVP"), 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(glGetUniformLocation(m_shaderProgram, "uModel"), 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(glGetUniformLocation(m_shaderProgram, "uBaseColor"), 1, glm::value_ptr(baseColor));
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uMetallic"), metallic);
    glUniform1f(glGetUniformLocation(m_shaderProgram, "uRoughness"), roughness);
    
    // Draw the cube
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(cubeIndices.size()), GL_UNSIGNED_INT, 0);
}

void OpenGLRenderer::EndFrame() {
    if (!m_initialized) return;
    
    // Swap buffers
    glfwSwapBuffers(m_window);
}

RenderStats OpenGLRenderer::Stats() const {
    return m_stats;
}

bool OpenGLRenderer::CreateShaders() {
    // Create vertex shader
    unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);
    
    // Check for shader compile errors
    int success;
    char infoLog[512];
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        NOVA_ERROR("Vertex shader compilation failed: " + std::string(infoLog));
        return false;
    }
    
    // Create fragment shader
    unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);
    
    // Check for shader compile errors
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        NOVA_ERROR("Fragment shader compilation failed: " + std::string(infoLog));
        return false;
    }
    
    // Create shader program
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vertexShader);
    glAttachShader(m_shaderProgram, fragmentShader);
    glLinkProgram(m_shaderProgram);
    
    // Check for linking errors
    glGetProgramiv(m_shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(m_shaderProgram, 512, NULL, infoLog);
        NOVA_ERROR("Shader program linking failed: " + std::string(infoLog));
        return false;
    }
    
    // Delete shaders as they're linked into our program now and no longer necessary
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    
    return true;
}

bool OpenGLRenderer::CreateBuffers() {
    // Create vertex array object
    glGenVertexArrays(1, &m_vertexArray);
    glBindVertexArray(m_vertexArray);
    
    // Create vertex buffer
    glGenBuffers(1, &m_vertexBuffer);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertexBuffer);
    glBufferData(GL_ARRAY_BUFFER, cubeVertices.size() * sizeof(Vertex), cubeVertices.data(), GL_STATIC_DRAW);
    
    // Create index buffer
    glGenBuffers(1, &m_indexBuffer);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_indexBuffer);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, cubeIndices.size() * sizeof(unsigned int), cubeIndices.data(), GL_STATIC_DRAW);
    
    // Set vertex attributes
    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);
    
    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    glEnableVertexAttribArray(1);
    
    // Texture coordinate attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, uv));
    glEnableVertexAttribArray(2);
    
    return true;
}

void OpenGLRenderer::SetupCamera() {
    // Setup projection matrix
    m_projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    
    // Setup view matrix
    m_view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
}

} // namespace nova