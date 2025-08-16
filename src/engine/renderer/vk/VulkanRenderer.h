#pragma once

// Ensure volk defines VK_NO_PROTOTYPES before including Vulkan headers
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <volk.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <cstdint>
#include <glm/glm.hpp>
#include "renderer/shadows/ShadowSystem.h"
#include "core/Log.h"

// Bounds-checked indexing helper
template<typename T>
T& idx(std::vector<T>& v, size_t i, const char* name) {
    if (i >= v.size()) {
        NOVA_ERROR("Index OOB: " + std::string(name) + "[" + std::to_string(i) + "] (size=" + std::to_string(v.size()) + ")");
        throw std::out_of_range(name);
    }
    return v[i];
}

namespace nova {

class VulkanRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() = default;

    // Prevent copying and moving to avoid vector splits
    VulkanRenderer(const VulkanRenderer&) = delete;
    VulkanRenderer& operator=(const VulkanRenderer&) = delete;
    VulkanRenderer(VulkanRenderer&&) = delete;
    VulkanRenderer& operator=(VulkanRenderer&&) = delete;

    // Core initialization
    void Init(GLFWwindow* window);
    void Shutdown();
    
    // Debug utilities
    void CreateDebugMessenger();
    void DestroyDebugMessenger();
    void SetDebugName(VkObjectType objectType, uint64_t objectHandle, const std::string& name);
    void SetDebugName(VkSemaphore semaphore, const std::string& name);
    void SetDebugName(VkFence fence, const std::string& name);
    void SetDebugName(VkCommandBuffer cmdBuffer, const std::string& name);
    
    // Window reference
    GLFWwindow* m_window = nullptr;
    
    // ImGui lifecycle
    void InitImGui(GLFWwindow* window);
    void BeginFrame();
    void EndFrame(VkCommandBuffer cmd);
    void EndFrame();
    VkCommandBuffer GetActiveCmd() const;
    bool IsImGuiReady() const;

    // 3D rendering
    void RenderFrame(class Camera* camera = nullptr, class LightingManager* lightingManager = nullptr);
    void UpdateMVP(const glm::mat4& mvp);
    void UpdateMVP(float deltaTime);
    
    // Asset system integration
    void SetAssetData(const std::vector<float>& vertexData, const std::vector<uint32_t>& indices);
    void SetInstanceData(const std::vector<glm::mat4>& instanceMatrices);
    void SetLights(const std::vector<glm::vec4>& lightPositions, const std::vector<glm::vec4>& lightColors);
    void UpdateLight(int lightIndex, const glm::vec3& position, float intensity);
    void UpdateLightInManager(int lightIndex, const glm::vec3& position, float intensity, class LightingManager* lightingManager);
    void SetLightsFromManager(class LightingManager* lightingManager);
    void RecreateSwapchain();
    void ToggleFullscreen();
    void WaitForDeviceIdle();
    
    // Performance tracking
    void UpdatePerformanceMetrics(double deltaTime);
    double GetFPS() const { return m_fps; }
    double GetFrameTime() const { return m_frameTime; }
    int GetFrameCount() const { return m_frameCount; }
    
    // Device properties
    VkDeviceSize GetMinUniformBufferOffsetAlignment() const { return m_minUniformBufferOffsetAlignment; }
    
    // Frame-in-flight management
    static constexpr uint32_t MAX_FRAMES_IN_FLIGHT = 2;
    uint32_t m_currentFrame = 0;
    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;
    std::vector<VkFence> m_imagesInFlight;
    std::vector<VkCommandBuffer> m_commandBuffers;
    
    // Device properties
    VkDeviceSize m_minUniformBufferOffsetAlignment = 0;

private:
    // Core Vulkan objects
    VkInstance    m_instance     = VK_NULL_HANDLE;
    VkPhysicalDevice m_phys      = VK_NULL_HANDLE;
    VkDevice      m_dev          = VK_NULL_HANDLE;
    uint32_t      m_queueFamily  = 0;
    VkQueue       m_queue        = VK_NULL_HANDLE;
    VkCommandPool m_cmdPool      = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer  = VK_NULL_HANDLE;
    VkFence       m_fence        = VK_NULL_HANDLE;
    VkSemaphore   m_semaphore    = VK_NULL_HANDLE;
    VkSurfaceKHR  m_surface      = VK_NULL_HANDLE;
    
    // Debug messenger
    VkDebugUtilsMessengerEXT m_debugMessenger = VK_NULL_HANDLE;

    // Swapchain core
    VkSwapchainKHR m_swapchain   = VK_NULL_HANDLE;
    std::vector<VkImage> m_swapchainImages;
    std::vector<VkImageView> m_swapchainImageViews;
    std::vector<VkFramebuffer> m_framebuffers;
    VkRenderPass   m_renderPass  = VK_NULL_HANDLE;
    VkExtent2D     m_extent      = {0, 0};
    VkFormat       m_format      = VK_FORMAT_UNDEFINED;

    // Pipeline
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkDescriptorSetLayout m_descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool m_descriptorPool = VK_NULL_HANDLE;
    std::vector<VkDescriptorSet> m_descriptorSets;

    // Vertex buffer
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexMemory = VK_NULL_HANDLE;
    uint32_t m_indexCount = 0;
    
    // Instance buffer for GPU instancing
    VkBuffer m_instanceBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_instanceMemory = VK_NULL_HANDLE;
    uint32_t m_instanceCount = 0;

    // Uniform buffer for MVP matrix
    VkBuffer m_uniformBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_uniformMemory = VK_NULL_HANDLE;
    void* m_uniformMapped = nullptr;
    
    // Light buffer
    VkBuffer m_lightBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_lightMemory = VK_NULL_HANDLE;
    void* m_lightMapped = nullptr;
    uint32_t m_lightCount = 0;
    std::vector<glm::vec4> m_lightPositions;
    std::vector<glm::vec4> m_lightColors;

    // Depth buffer
    VkImage m_depthImage = VK_NULL_HANDLE;
    VkDeviceMemory m_depthImageMemory = VK_NULL_HANDLE;
    VkImageView m_depthImageView = VK_NULL_HANDLE;

    // ImGui
    bool          m_imguiReady = false;
    VkDescriptorPool m_imguiDescriptorPool = VK_NULL_HANDLE;
    
    // Window state
    bool          m_isFullscreen = false;
    bool          m_fullscreenToggleInProgress = false;
    
    // Performance tracking
    double        m_lastFrameTime = 0.0;
    double        m_frameTime = 0.0;
    double        m_fps = 60.0;
    int           m_frameCount = 0;
    double        m_fpsUpdateTime = 0.0;
    
    // Current MVP matrix for light updates
    glm::mat4     m_currentMVP = glm::mat4(1.0f);

    // Shadow system
    ShadowSystem m_shadowSystem;
    VkRenderPass m_currentRenderPass = VK_NULL_HANDLE;
    
    // Helper functions
    void CreateInstance();
    void CreateDevice();
    void CreateSwapchain();
    void CreateRenderPass();
    void CreateFramebuffers();
    void CreateDepthResources();
    void CreatePipeline();
    void CreateVertexBuffer();
    void CreateInstanceBuffer();
    void CreateUniformBuffer();
    void CreateLightBuffer();
    void CreateCommandPool();
    void CreateSyncObjects();
    void CreateDescriptorPool();
    void CreateDescriptorSets();
    void SyncPerImageVectors(uint32_t count);
    void SanitySwapchainSizes();
    void LogSwapchainSizes(const std::string& context);
    void CreateShadowResources();
    void CreateShadowPipeline();
    void CreateShadowDescriptorSet();
    void RenderShadowMaps(VkCommandBuffer cmd, int lightIndex);
    glm::mat4 CalculateLightSpaceMatrix(const glm::vec3& lightPos);
    glm::mat4 CalculateLightSpaceMatrixForFace(const glm::vec3& lightPos, int face);
    void RecordCommandBuffer(VkCommandBuffer cmd, VkFramebuffer framebuffer, class Camera* camera = nullptr);
    void RenderUI(class Camera* camera = nullptr, class LightingManager* lightingManager = nullptr);
    
    // Utility functions
    uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer commandBuffer);
};

} // namespace nova




