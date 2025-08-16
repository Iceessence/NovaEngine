#pragma once

#include <volk.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace nova {

// Forward declarations
class VulkanRenderer;

enum class ShadowType {
    Directional,
    Spot,
    Point
};

struct ShadowCascade {
    glm::mat4 viewProjection;
    float splitDepth;
    float texelSize;
    uint32_t arrayLayer;
};

struct ShadowLight {
    ShadowType type;
    glm::vec3 position;
    glm::vec3 direction;
    float range;
    float fov;
    bool castShadows;
    float shadowBias;
    float shadowBiasSlope;
    
    // Shadow map allocation
    uint32_t shadowMapIndex;
    uint32_t arrayLayer;
    uint32_t resolution;
    
    // For directional lights (CSM)
    std::vector<ShadowCascade> cascades;
    
    // For point lights
    uint32_t cubeMapIndex;
};

class ShadowSystem {
public:
    ShadowSystem();
    ~ShadowSystem();
    
    // Initialization
    void Initialize(VkDevice device, VkPhysicalDevice physicalDevice);
    void Shutdown();
    
    // Shadow map management
    void CreateShadowMaps();
    void DestroyShadowMaps();
    
    // Light management
    void AddLight(const ShadowLight& light);
    void RemoveLight(uint32_t lightIndex);
    void UpdateLight(uint32_t lightIndex, const ShadowLight& light);
    
    // Shadow rendering
    void RenderShadowMaps(VkCommandBuffer cmd, const std::vector<ShadowLight>& lights);
    
    // Descriptor management
    VkDescriptorSetLayout GetShadowDescriptorSetLayout() const { return m_shadowDescriptorSetLayout; }
    VkDescriptorSet GetShadowDescriptorSet() const { return m_shadowDescriptorSet; }
    
    // Getters
    uint32_t GetMaxDirectionalLights() const { return MAX_DIRECTIONAL_LIGHTS; }
    uint32_t GetMaxSpotLights() const { return MAX_SPOT_LIGHTS; }
    uint32_t GetMaxPointLights() const { return MAX_POINT_LIGHTS; }
    
    // Debug
    void RenderDebugUI();

private:
    // Constants
    static const uint32_t MAX_DIRECTIONAL_LIGHTS = 1;
    static const uint32_t MAX_SPOT_LIGHTS = 4;
    static const uint32_t MAX_POINT_LIGHTS = 2;
    static const uint32_t SHADOW_MAP_SIZE = 1024;
    static const uint32_t CASCADE_COUNT = 3;
    
    // Device
    VkDevice m_device = VK_NULL_HANDLE;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    
    // 2D Shadow maps (directional + spot lights)
    VkImage m_shadowMap2D = VK_NULL_HANDLE;
    VkDeviceMemory m_shadowMap2DMemory = VK_NULL_HANDLE;
    VkImageView m_shadowMap2DView = VK_NULL_HANDLE;
    std::vector<VkImageView> m_shadowMap2DLayerViews;
    
    // Cubemap shadow maps (point lights)
    VkImage m_shadowMapCube = VK_NULL_HANDLE;
    VkDeviceMemory m_shadowMapCubeMemory = VK_NULL_HANDLE;
    VkImageView m_shadowMapCubeView = VK_NULL_HANDLE;
    std::vector<VkImageView> m_shadowMapCubeLayerViews;
    
    // Framebuffers
    std::vector<VkFramebuffer> m_shadowFramebuffers2D;
    std::vector<VkFramebuffer> m_shadowFramebuffersCube;
    
    // Render passes
    VkRenderPass m_shadowRenderPass2D = VK_NULL_HANDLE;
    VkRenderPass m_shadowRenderPassCube = VK_NULL_HANDLE;
    
    // Pipelines
    VkPipeline m_shadowPipeline2D = VK_NULL_HANDLE;
    VkPipeline m_shadowPipelineCube = VK_NULL_HANDLE;
    VkPipelineLayout m_shadowPipelineLayout = VK_NULL_HANDLE;
    
    // Descriptor sets
    VkDescriptorSetLayout m_shadowDescriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorSet m_shadowDescriptorSet = VK_NULL_HANDLE;
    VkDescriptorPool m_shadowDescriptorPool = VK_NULL_HANDLE;
    VkSampler m_shadowSampler = VK_NULL_HANDLE;
    
    // Light tracking
    std::vector<ShadowLight> m_lights;
    
    // Helper functions
    void CreateShadowRenderPasses();
    void CreateShadowPipelines();
    void CreateShadowDescriptorSet();
    void CreateShadowSampler();
    void CreateShadowFramebuffers();
    void TransitionImagesToReadOnly();
    
    void RenderDirectionalShadowMaps(VkCommandBuffer cmd, const ShadowLight& light);
    void RenderSpotShadowMaps(VkCommandBuffer cmd, const ShadowLight& light);
    void RenderPointShadowMaps(VkCommandBuffer cmd, const ShadowLight& light);
    
    glm::mat4 CalculateCascadeMatrix(const ShadowLight& light, uint32_t cascadeIndex);
    glm::mat4 CalculateSpotLightMatrix(const ShadowLight& light);
    glm::mat4 CalculatePointLightMatrix(const ShadowLight& light, uint32_t face);
    
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
};

} // namespace nova
