#pragma once

// Ensure volk defines VK_NO_PROTOTYPES before including Vulkan headers
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <volk.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <vector>
#include <cstdint>

namespace nova {

class VulkanRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() = default;

    // Core Vulkan initialization
    bool Init(GLFWwindow* window);
    void Shutdown();
    
    // ImGui lifecycle
    void InitImGui(GLFWwindow* window);
    void BeginFrame();
    void EndFrame();
    
    // Rendering
    void BeginRender();
    void DrawCube(const glm::mat4& model, const glm::vec3& baseColor, float metallic, float roughness);
    void EndRender();
    
    // Utility
    bool IsImGuiReady() const { return m_imguiReady; }
    VkCommandBuffer GetActiveCmd() const;

private:
    // Helper functions
    void CreateVertexBuffer();
    void CreateIndexBuffer();
    void CreateRenderPass();
    void CreateGraphicsPipeline();
    // Vulkan state
    bool          m_imguiReady = false;
    bool          m_initialized = false;

    VkInstance    m_instance     = VK_NULL_HANDLE;
    VkPhysicalDevice m_phys      = VK_NULL_HANDLE;
    VkDevice      m_dev          = VK_NULL_HANDLE;
    uint32_t      m_queueFamily  = 0;
    VkQueue       m_queue        = VK_NULL_HANDLE;

    std::vector<VkImage> m_images;
    VkRenderPass   m_renderPass  = VK_NULL_HANDLE;
    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
    
    // Command buffer management
    VkCommandPool m_cmdPool = VK_NULL_HANDLE;
    VkCommandBuffer m_cmdBuffer = VK_NULL_HANDLE;
    
    // Swapchain
    VkSwapchainKHR m_swapchain = VK_NULL_HANDLE;
    std::vector<VkImageView> m_imageViews;
    VkFramebuffer m_framebuffer = VK_NULL_HANDLE;
    
    // Synchronization
    VkSemaphore m_imageAvailableSemaphore = VK_NULL_HANDLE;
    VkSemaphore m_renderFinishedSemaphore = VK_NULL_HANDLE;
    VkFence m_inFlightFence = VK_NULL_HANDLE;
    
    // Pipeline
    VkPipeline m_pipeline = VK_NULL_HANDLE;
    VkPipelineLayout m_pipelineLayout = VK_NULL_HANDLE;
    
    // Vertex buffer for cube
    VkBuffer m_vertexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_vertexBufferMemory = VK_NULL_HANDLE;
    VkBuffer m_indexBuffer = VK_NULL_HANDLE;
    VkDeviceMemory m_indexBufferMemory = VK_NULL_HANDLE;
};

} // namespace nova




