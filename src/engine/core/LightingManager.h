#pragma once

#include "Light.h"
#include <vector>
#include <memory>

namespace nova {

class LightingManager {
public:
    LightingManager();
    ~LightingManager() = default;
    
    // Light management
    void AddLight(const Light& light);
    void RemoveLight(size_t index);
    void ClearLights();
    
    // Light access
    const std::vector<Light>& GetLights() const { return m_lights; }
    size_t GetLightCount() const { return m_lights.size(); }
    
    // Convenience methods for common light setups
    void SetupDefaultLighting();
    void SetupThreePointLighting();
    void SetupDramaticLighting();
    
    // Light updates
    void UpdateLights(float deltaTime);
    
    // Getters for specific light types
    std::vector<Light> GetDirectionalLights() const;
    std::vector<Light> GetPointLights() const;
    std::vector<Light> GetSpotLights() const;

private:
    std::vector<Light> m_lights;
    float m_time = 0.0f;
};

} // namespace nova



