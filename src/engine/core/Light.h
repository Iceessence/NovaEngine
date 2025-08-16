#pragma once

#include <glm/glm.hpp>

namespace nova {

enum class LightType {
    Directional = 0,
    Point = 1,
    Spot = 2
};

struct Light {
    LightType type = LightType::Point;
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction = glm::vec3(0.0f, -1.0f, 0.0f);
    glm::vec3 color = glm::vec3(1.0f, 1.0f, 1.0f);
    float intensity = 1.0f;
    float range = 10.0f;
    float spotAngle = 45.0f;
    float spotBlend = 0.1f;
    
    // Attenuation for point/spot lights
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;
    
    // Shadow properties
    bool castShadows = false;
    float shadowBias = 0.005f;
    
    Light() = default;
    
    Light(LightType lightType, const glm::vec3& pos, const glm::vec3& col, float intens = 1.0f)
        : type(lightType), position(pos), color(col), intensity(intens) {}
    
    // Directional light constructor
    static Light CreateDirectional(const glm::vec3& dir, const glm::vec3& col, float intens = 1.0f) {
        Light light(LightType::Directional, glm::vec3(0.0f), col, intens);
        light.direction = glm::normalize(dir);
        return light;
    }
    
    // Point light constructor
    static Light CreatePoint(const glm::vec3& pos, const glm::vec3& col, float intens = 1.0f, float rng = 10.0f) {
        Light light(LightType::Point, pos, col, intens);
        light.range = rng;
        return light;
    }
    
    // Spot light constructor
    static Light CreateSpot(const glm::vec3& pos, const glm::vec3& dir, const glm::vec3& col, 
                          float intens = 1.0f, float angle = 45.0f, float blend = 0.1f) {
        Light light(LightType::Spot, pos, col, intens);
        light.direction = glm::normalize(dir);
        light.spotAngle = angle;
        light.spotBlend = blend;
        return light;
    }
};

} // namespace nova



