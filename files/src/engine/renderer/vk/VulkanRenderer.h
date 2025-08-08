#pragma once

// Ensure volk defines VK_NO_PROTOTYPES before including Vulkan headers
#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <volk.h>
#include <vector>
#include <cstdint>

namespace nova {

class VulkanRenderer {
public:
    VulkanRenderer() = default;
    ~VulkanRenderer() = default;

    // ImGui lifecycle used by VulkanRenderer.cpp
    void InitImGui();
    void BeginFrame();
    void EndFrame(VkCommandBuffer cmd);

    // Safe inline getter used by .cpp
    bool IsImGuiReady() const { return m_imguiReady; }

private:
    // Minimal state expected by the .cpp
    bool          m_imguiReady = false;

    VkInstance    m_instance     = VK_NULL_HANDLE;
    VkPhysicalDevice m_phys      = VK_NULL_HANDLE;
    VkDevice      m_dev          = VK_NULL_HANDLE;
    uint32_t      m_queueFamily  = 0;
    VkQueue       m_queue        = VK_NULL_HANDLE;

    std::vector<VkImage> m_images;
    VkRenderPass   m_renderPass  = VK_NULL_HANDLE;
    VkDescriptorPool m_imguiPool = VK_NULL_HANDLE;
};

} // namespace nova
