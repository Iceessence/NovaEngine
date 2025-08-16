#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct GLFWwindow;

namespace nova {

class Camera {
public:
    Camera();
    ~Camera() = default;

    // Camera properties
    void SetPosition(const glm::vec3& position);
    void SetTarget(const glm::vec3& target);
    void SetUp(const glm::vec3& up);
    
    // Camera controls
    void Update(float deltaTime, GLFWwindow* window);
    void ProcessMouseMovement(float xoffset, float yoffset, bool constrainPitch = true);
    void ProcessMouseScroll(float yoffset);
    void ProcessKeyboard(float deltaTime, GLFWwindow* window);
    
    // Matrix generation
    glm::mat4 GetViewMatrix() const;
    glm::mat4 GetProjectionMatrix() const;
    glm::mat4 GetViewProjectionMatrix() const;
    
    // Getters
    glm::vec3 GetPosition() const { return m_position; }
    glm::vec3 GetFront() const { return m_front; }
    glm::vec3 GetRight() const { return m_right; }
    glm::vec3 GetUp() const { return m_up; }
    float GetFOV() const { return m_fov; }
    float GetNearPlane() const { return m_nearPlane; }
    float GetFarPlane() const { return m_farPlane; }
    float GetMovementSpeed() const { return m_movementSpeed; }
    
    // Setters
    void SetFOV(float fov) { m_fov = fov; }
    void SetAspectRatio(float aspectRatio) { m_aspectRatio = aspectRatio; }
    void SetNearPlane(float nearPlane) { m_nearPlane = nearPlane; }
    void SetFarPlane(float farPlane) { m_farPlane = farPlane; }
    void SetMovementSpeed(float speed) { m_movementSpeed = speed; }
    void SetMouseSensitivity(float sensitivity) { m_mouseSensitivity = sensitivity; }

private:
    void UpdateCameraVectors();
    
    // Camera attributes
    glm::vec3 m_position = glm::vec3(0.0f, 0.0f, 3.0f);
    glm::vec3 m_front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 m_up = glm::vec3(0.0f, 1.0f, 0.0f);
    glm::vec3 m_right = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 m_worldUp = glm::vec3(0.0f, 1.0f, 0.0f);
    
    // Euler angles
    float m_yaw = -90.0f;
    float m_pitch = 0.0f;
    
    // Camera options
    float m_movementSpeed = 5.0f;
    float m_mouseSensitivity = 0.1f;
    float m_fov = 45.0f;
    float m_aspectRatio = 16.0f / 9.0f;
    float m_nearPlane = 0.1f;
    float m_farPlane = 100.0f;
    
    // Mouse state
    bool m_firstMouse = true;
    float m_lastX = 0.0f;
    float m_lastY = 0.0f;
};

} // namespace nova
