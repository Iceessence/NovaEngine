#include "LightingManager.h"
#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>

namespace nova {

LightingManager::LightingManager() {
    SetupDefaultLighting();
}

void LightingManager::AddLight(const Light& light) {
    m_lights.push_back(light);
}

void LightingManager::RemoveLight(size_t index) {
    if (index < m_lights.size()) {
        m_lights.erase(m_lights.begin() + index);
    }
}

void LightingManager::ClearLights() {
    m_lights.clear();
}

void LightingManager::SetupDefaultLighting() {
    ClearLights();
    
    // Main directional light (sun)
    auto sunLight = Light::CreateDirectional(
        glm::vec3(0.5f, -1.0f, 0.3f),  // Direction
        glm::vec3(1.0f, 0.95f, 0.8f),  // Warm sunlight color
        1.0f                           // Intensity
    );
    AddLight(sunLight);
    
    // Fill light from opposite direction
    auto fillLight = Light::CreateDirectional(
        glm::vec3(-0.3f, -0.5f, -0.8f), // Direction
        glm::vec3(0.6f, 0.7f, 1.0f),    // Cool blue fill
        0.3f                            // Lower intensity
    );
    AddLight(fillLight);
    
    // Ambient point light
    auto ambientLight = Light::CreatePoint(
        glm::vec3(0.0f, 5.0f, 0.0f),    // Position above
        glm::vec3(0.8f, 0.8f, 1.0f),    // Slightly cool white
        0.5f,                           // Moderate intensity
        15.0f                           // Large range
    );
    AddLight(ambientLight);
}

void LightingManager::SetupThreePointLighting() {
    ClearLights();
    
    // Key light (main light)
    auto keyLight = Light::CreatePoint(
        glm::vec3(5.0f, 3.0f, 2.0f),    // Position
        glm::vec3(1.0f, 0.95f, 0.8f),   // Warm key light
        1.0f,                           // High intensity
        12.0f                           // Range
    );
    AddLight(keyLight);
    
    // Fill light (softer, opposite side)
    auto fillLight = Light::CreatePoint(
        glm::vec3(-4.0f, 2.0f, -3.0f),  // Position
        glm::vec3(0.7f, 0.8f, 1.0f),    // Cool fill light
        0.4f,                           // Lower intensity
        10.0f                           // Range
    );
    AddLight(fillLight);
    
    // Back light (rim lighting)
    auto backLight = Light::CreatePoint(
        glm::vec3(0.0f, 4.0f, -5.0f),   // Position behind
        glm::vec3(1.0f, 1.0f, 1.0f),    // White back light
        0.6f,                           // Medium intensity
        8.0f                            // Range
    );
    AddLight(backLight);
}

void LightingManager::SetupDramaticLighting() {
    ClearLights();
    
    // Single dramatic light from above
    auto dramaticLight = Light::CreateSpot(
        glm::vec3(0.0f, 8.0f, 0.0f),    // Position above
        glm::vec3(0.0f, -1.0f, 0.0f),   // Pointing down
        glm::vec3(1.0f, 0.9f, 0.7f),    // Warm dramatic color
        2.0f,                           // High intensity
        30.0f,                          // Narrow angle
        0.2f                            // Sharp falloff
    );
    AddLight(dramaticLight);
    
    // Subtle rim light
    auto rimLight = Light::CreatePoint(
        glm::vec3(-3.0f, 1.0f, -2.0f),  // Position
        glm::vec3(0.3f, 0.4f, 0.8f),    // Blue rim light
        0.3f,                           // Low intensity
        6.0f                            // Short range
    );
    AddLight(rimLight);
}

void LightingManager::UpdateLights(float deltaTime) {
    m_time += deltaTime;
    
    // Animate lights if we have any
    if (m_lights.empty()) return;
    
    // Rotate the first light around the scene
    if (m_lights.size() > 0) {
        float radius = 8.0f;
        float speed = 0.5f;
        m_lights[0].position.x = cos(m_time * speed) * radius;
        m_lights[0].position.z = sin(m_time * speed) * radius;
        m_lights[0].position.y = 3.0f + sin(m_time * speed * 2.0f) * 1.0f;
    }
    
    // Pulse the second light if it exists
    if (m_lights.size() > 1) {
        float pulse = 0.5f + 0.5f * sin(m_time * 3.0f);
        m_lights[1].intensity = pulse;
    }
    
    // Rotate the third light if it exists
    if (m_lights.size() > 2) {
        float radius = 6.0f;
        float speed = 0.3f;
        m_lights[2].position.x = cos(m_time * speed + 2.0f) * radius;
        m_lights[2].position.z = sin(m_time * speed + 2.0f) * radius;
        m_lights[2].position.y = 2.0f;
    }
}

std::vector<Light> LightingManager::GetDirectionalLights() const {
    std::vector<Light> directionalLights;
    for (const auto& light : m_lights) {
        if (light.type == LightType::Directional) {
            directionalLights.push_back(light);
        }
    }
    return directionalLights;
}

std::vector<Light> LightingManager::GetPointLights() const {
    std::vector<Light> pointLights;
    for (const auto& light : m_lights) {
        if (light.type == LightType::Point) {
            pointLights.push_back(light);
        }
    }
    return pointLights;
}

std::vector<Light> LightingManager::GetSpotLights() const {
    std::vector<Light> spotLights;
    for (const auto& light : m_lights) {
        if (light.type == LightType::Spot) {
            spotLights.push_back(light);
        }
    }
    return spotLights;
}

} // namespace nova



