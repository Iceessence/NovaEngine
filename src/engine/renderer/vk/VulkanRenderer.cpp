#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif

#include <volk.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <array>
#include <cstring>
#include <stdexcept>
#include <thread>
#include <chrono>

#include "VulkanRenderer.h"
#include "VulkanHelpers.h"
#include "core/Log.h"
#include "core/Camera.h"
#include "core/LightingManager.h"

namespace nova {

// Vertex data structure (for reference only - not used with SetAssetData)
struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 uv;
};

// Push constants (≤256 bytes for best compatibility)
struct PushConstants {
    glm::mat4 viewProjection;  // 64 bytes
    glm::vec4 baseColor;       // 16 bytes
    float metallic;            // 4 bytes
    float roughness;           // 4 bytes
    // Total: 88 bytes (well under 256 byte limit)
};

// Uniform buffer data (moved from push constants)
struct UniformBufferObject {
    glm::mat4 model;           // Model matrix (moved from push constants)
    glm::vec4 lightPositions[3];  // 3 light positions
    glm::vec4 lightColors[3];     // 3 light colors
    glm::mat4 lightSpaceMatrices[3];   // Light space matrices for all lights
};

void VulkanRenderer::Init(GLFWwindow* window) {
    NOVA_INFO("Initializing Vulkan renderer...");
    
    m_window = window;
    NOVA_INFO("Window set, initializing volk...");
    volkInitialize();
    NOVA_INFO("Volk initialized, creating instance...");
    CreateInstance();
    NOVA_INFO("Instance created, creating device...");
    CreateDevice();
    NOVA_INFO("Device created, creating swapchain...");
    CreateSwapchain();
    NOVA_INFO("Swapchain created, creating render pass...");
    CreateRenderPass();
    NOVA_INFO("Render pass created, creating framebuffers...");
    CreateFramebuffers();
    NOVA_INFO("Framebuffers created, creating uniform buffer...");
    CreateUniformBuffer();
    NOVA_INFO("Uniform buffer created, creating light buffer...");
    CreateLightBuffer();
    NOVA_INFO("Light buffer created, creating command pool...");
    CreateCommandPool();
    NOVA_INFO("Command pool created, creating sync objects...");
    CreateSyncObjects();
    NOVA_INFO("Sync objects created, skipping shadow system initialization...");
    // m_shadowSystem.Initialize(m_dev, m_phys); // Temporarily disabled to prevent crashes
    NOVA_INFO("Shadow system initialization skipped, creating pipeline...");
    CreatePipeline(); // Move this after shadow resources are created
    NOVA_INFO("Pipeline created");
    
    // Log sizes after full initialization
    LogSwapchainSizes("After full initialization");
    
    NOVA_INFO("Vulkan renderer initialized successfully");
}

void VulkanRenderer::CreateInstance() {
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "NovaEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NovaEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    std::vector<const char*> extensions = {
        VK_KHR_SURFACE_EXTENSION_NAME,
#ifdef _WIN32
        VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
        VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
    createInfo.ppEnabledExtensionNames = extensions.data();

    // Enable validation layers in debug builds
#ifdef _DEBUG
    std::vector<const char*> validationLayers = {
        "VK_LAYER_KHRONOS_validation"
    };
    createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();
    
    // Enable validation features
    VkValidationFeaturesEXT validationFeatures{};
    validationFeatures.sType = VK_STRUCTURE_TYPE_VALIDATION_FEATURES_EXT;
    validationFeatures.enabledValidationFeatureCount = 2;
    const VkValidationFeatureEnableEXT enabledFeatures[] = {
        VK_VALIDATION_FEATURE_ENABLE_BEST_PRACTICES_EXT,
        VK_VALIDATION_FEATURE_ENABLE_SYNCHRONIZATION_VALIDATION_EXT
    };
    validationFeatures.pEnabledValidationFeatures = enabledFeatures;
    createInfo.pNext = &validationFeatures;
    
    NOVA_INFO("Vulkan validation layers enabled");
#endif

    VK_CHECK(vkCreateInstance(&createInfo, nullptr, &m_instance));
    volkLoadInstance(m_instance);
    
    // Create debug messenger
    CreateDebugMessenger();
}

void VulkanRenderer::CreateDebugMessenger() {
#ifdef _DEBUG
    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
                                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
                             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = [](VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, 
                                   VkDebugUtilsMessageTypeFlagsEXT messageType, 
                                   const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, 
                                   void* pUserData) -> VkBool32 {
        if (messageSeverity >= VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            NOVA_ERROR("Vulkan Validation: " + std::string(pCallbackData->pMessage));
        } else {
            NOVA_INFO("Vulkan Debug: " + std::string(pCallbackData->pMessage));
        }
        return VK_FALSE;
    };
    
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        VK_CHECK(func(m_instance, &createInfo, nullptr, &m_debugMessenger));
        NOVA_INFO("Vulkan debug messenger created");
    } else {
        NOVA_WARN("Vulkan debug messenger creation function not available");
    }
#endif
}

void VulkanRenderer::DestroyDebugMessenger() {
#ifdef _DEBUG
    if (m_debugMessenger != VK_NULL_HANDLE) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(m_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(m_instance, m_debugMessenger, nullptr);
        }
        m_debugMessenger = VK_NULL_HANDLE;
    }
#endif
}

void VulkanRenderer::SetDebugName(VkObjectType objectType, uint64_t objectHandle, const std::string& name) {
#ifdef _DEBUG
    VkDebugUtilsObjectNameInfoEXT nameInfo{};
    nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
    nameInfo.objectType = objectType;
    nameInfo.objectHandle = objectHandle;
    nameInfo.pObjectName = name.c_str();
    
    auto func = (PFN_vkSetDebugUtilsObjectNameEXT)vkGetInstanceProcAddr(m_instance, "vkSetDebugUtilsObjectNameEXT");
    if (func != nullptr) {
        func(m_dev, &nameInfo);
    }
#endif
}

void VulkanRenderer::SetDebugName(VkSemaphore semaphore, const std::string& name) {
    SetDebugName(VK_OBJECT_TYPE_SEMAPHORE, (uint64_t)semaphore, name);
}

void VulkanRenderer::SetDebugName(VkFence fence, const std::string& name) {
    SetDebugName(VK_OBJECT_TYPE_FENCE, (uint64_t)fence, name);
}

void VulkanRenderer::SetDebugName(VkCommandBuffer cmdBuffer, const std::string& name) {
    SetDebugName(VK_OBJECT_TYPE_COMMAND_BUFFER, (uint64_t)cmdBuffer, name);
}

void VulkanRenderer::CreateDevice() {
    // Find physical device
    uint32_t deviceCount = 0;
    VkResult enumResult = vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);
    if (enumResult != VK_SUCCESS || deviceCount == 0) {
        NOVA_ERROR("Failed to find Vulkan devices");
        throw std::runtime_error("Failed to find Vulkan devices");
    }
    
    std::vector<VkPhysicalDevice> devices(deviceCount);
    enumResult = vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());
    if (enumResult != VK_SUCCESS) {
        NOVA_ERROR("Failed to enumerate Vulkan devices");
        throw std::runtime_error("Failed to enumerate Vulkan devices");
    }
    
    m_phys = devices[0]; // Use first available device
    NOVA_INFO("Selected physical device: " + std::to_string(deviceCount) + " devices available");
    
    // Find queue family that supports both graphics and presentation
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(m_phys, &queueFamilyCount, nullptr);
    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(m_phys, &queueFamilyCount, queueFamilies.data());
    
    // Create a temporary surface to check presentation support
    VkSurfaceKHR tempSurface;
    glfwCreateWindowSurface(m_instance, m_window, nullptr, &tempSurface);
    
    bool foundSuitableQueueFamily = false;
    for (uint32_t i = 0; i < queueFamilyCount; i++) {
        if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            VkBool32 presentSupport = false;
            VkResult presentResult = vkGetPhysicalDeviceSurfaceSupportKHR(m_phys, i, tempSurface, &presentSupport);
            if (presentResult == VK_SUCCESS && presentSupport) {
                m_queueFamily = i;
                foundSuitableQueueFamily = true;
                NOVA_INFO("Found suitable queue family: " + std::to_string(i));
                break;
            }
        }
    }
    
    if (!foundSuitableQueueFamily) {
        throw std::runtime_error("Failed to find a suitable queue family");
    }
    
    vkDestroySurfaceKHR(m_instance, tempSurface, nullptr);
    
    // Query physical device features and properties
    VkPhysicalDeviceFeatures2 features2{};
    features2.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.timelineSemaphore = VK_TRUE;
    vulkan12Features.descriptorIndexing = VK_TRUE;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE;
    vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    features2.pNext = &vulkan12Features;
    
    vkGetPhysicalDeviceFeatures2(m_phys, &features2);
    
    // Log enabled features
    NOVA_INFO("Device features enabled:");
    NOVA_INFO("  samplerAnisotropy: " + std::string(features2.features.samplerAnisotropy ? "YES" : "NO"));
    NOVA_INFO("  geometryShader: " + std::string(features2.features.geometryShader ? "YES" : "NO"));
    NOVA_INFO("  tessellationShader: " + std::string(features2.features.tessellationShader ? "YES" : "NO"));
    NOVA_INFO("  multiDrawIndirect: " + std::string(features2.features.multiDrawIndirect ? "YES" : "NO"));
    NOVA_INFO("  timelineSemaphore: " + std::string(vulkan12Features.timelineSemaphore ? "YES" : "NO"));
    NOVA_INFO("  descriptorIndexing: " + std::string(vulkan12Features.descriptorIndexing ? "YES" : "NO"));
    
    // Get device properties for alignment requirements
    VkPhysicalDeviceProperties deviceProps{};
    vkGetPhysicalDeviceProperties(m_phys, &deviceProps);
    m_minUniformBufferOffsetAlignment = deviceProps.limits.minUniformBufferOffsetAlignment;
    NOVA_INFO("Device properties:");
    NOVA_INFO("  minUniformBufferOffsetAlignment: " + std::to_string(m_minUniformBufferOffsetAlignment));
    NOVA_INFO("  maxUniformBufferRange: " + std::to_string(deviceProps.limits.maxUniformBufferRange));
    NOVA_INFO("  maxStorageBufferRange: " + std::to_string(deviceProps.limits.maxStorageBufferRange));
    
    // Create logical device
    float queuePriority = 1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo{};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = m_queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;
    
    std::vector<const char*> deviceExtensions = {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME
    };
    
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = &queueCreateInfo;
    createInfo.queueCreateInfoCount = 1;
    createInfo.pNext = &features2; // Use modern feature chain
    createInfo.pEnabledFeatures = nullptr; // Not used when using pNext
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();
    
    VK_CHECK(vkCreateDevice(m_phys, &createInfo, nullptr, &m_dev));
    volkLoadDevice(m_dev);
    
    vkGetDeviceQueue(m_dev, m_queueFamily, 0, &m_queue);
    
    NOVA_INFO("Logical device created successfully with feature chain");
}

void VulkanRenderer::CreateSwapchain() {
    // Use existing surface or create new one
    VkSurfaceKHR surface = m_surface;
    if (surface == VK_NULL_HANDLE) {
        VkResult result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface);
        if (result != VK_SUCCESS) {
            NOVA_INFO("Failed to create window surface");
            throw std::runtime_error("Failed to create window surface");
        }
        m_surface = surface;
        NOVA_INFO("Window surface created successfully");
    }
    
    VkSurfaceCapabilitiesKHR capabilities;
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_phys, surface, &capabilities);
    
    // Choose surface format
    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_phys, surface, &formatCount, nullptr);
    if (formatCount == 0) {
        throw std::runtime_error("Failed to find surface formats");
    }
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(m_phys, surface, &formatCount, formats.data());
    
    m_format = formats[0].format;
    for (const auto& format : formats) {
        if (format.format == VK_FORMAT_B8G8R8A8_SRGB && format.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            m_format = format.format;
            break;
        }
    }
    
    // Choose extent
    if (capabilities.currentExtent.width != UINT32_MAX) {
        m_extent = capabilities.currentExtent;
    } else {
        int width, height;
        glfwGetFramebufferSize(m_window, &width, &height);
        m_extent.width = static_cast<uint32_t>(width);
        m_extent.height = static_cast<uint32_t>(height);
    }
    
    // Create swapchain
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface;
    createInfo.minImageCount = capabilities.minImageCount + 1;
    createInfo.imageFormat = m_format;
    createInfo.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
    createInfo.imageExtent = m_extent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = m_swapchain; // Use existing swapchain if recreating
    
    VK_CHECK(vkCreateSwapchainKHR(m_dev, &createInfo, nullptr, &m_swapchain));
    NOVA_INFO("Swapchain created successfully");
    
    // Get swapchain images
    uint32_t imageCount;
    vkGetSwapchainImagesKHR(m_dev, m_swapchain, &imageCount, nullptr);
    m_swapchainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(m_dev, m_swapchain, &imageCount, m_swapchainImages.data());
    NOVA_INFO("Swapchain images acquired: " + std::to_string(imageCount));
    
    // Sync all per-image vectors to the same size
    SyncPerImageVectors(imageCount);
    
    // Create image views
    m_swapchainImageViews.resize(m_swapchainImages.size());
    for (size_t i = 0; i < m_swapchainImages.size(); i++) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_swapchainImages[i];
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_format;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;
        
        VK_CHECK(vkCreateImageView(m_dev, &viewInfo, nullptr, &m_swapchainImageViews[i]));
    }
    
    // Log sizes after CreateSwapchain
    LogSwapchainSizes("After CreateSwapchain");
    
    // Sanity check all vectors are in sync
    SanitySwapchainSizes();
}

void VulkanRenderer::CreateRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = m_format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
    
    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = VK_FORMAT_D32_SFLOAT;
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
    
    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
    
    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;
    
    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
    
    VkAttachmentDescription attachments[] = {colorAttachment, depthAttachment};
    
    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachments;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;
    
    VK_CHECK(vkCreateRenderPass(m_dev, &renderPassInfo, nullptr, &m_renderPass));
    
    CreateDepthResources();
}

void VulkanRenderer::CreateFramebuffers() {
    m_framebuffers.resize(m_swapchainImageViews.size());
    
    for (size_t i = 0; i < m_swapchainImageViews.size(); i++) {
        VkImageView attachments[] = {m_swapchainImageViews[i], m_depthImageView};
        
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_renderPass;
        framebufferInfo.attachmentCount = 2;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = m_extent.width;
        framebufferInfo.height = m_extent.height;
        framebufferInfo.layers = 1;
        
        VK_CHECK(vkCreateFramebuffer(m_dev, &framebufferInfo, nullptr, &m_framebuffers[i]));
    }
}

void VulkanRenderer::CreateDepthResources() {
    VkFormat depthFormat = VK_FORMAT_D32_SFLOAT;
    
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = m_extent.width;
    imageInfo.extent.height = m_extent.height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = depthFormat;
    imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    
    VK_CHECK(vkCreateImage(m_dev, &imageInfo, nullptr, &m_depthImage));
    
    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(m_dev, m_depthImage, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_depthImageMemory));
    vkBindImageMemory(m_dev, m_depthImage, m_depthImageMemory, 0);
    
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = m_depthImage;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = depthFormat;
    viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;
    
    VK_CHECK(vkCreateImageView(m_dev, &viewInfo, nullptr, &m_depthImageView));
}

void VulkanRenderer::CreatePipeline() {
    NOVA_INFO("CreatePipeline: Loading shaders...");
    // Load shaders
    auto vertShader = vkutil::LoadShader(m_dev, "assets/shaders/pbr.vert.spv");
    auto fragShader = vkutil::LoadShader(m_dev, "assets/shaders/pbr.frag.spv");
    NOVA_INFO("CreatePipeline: Shaders loaded successfully");
    
    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShader.module;
    vertShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShader.module;
    fragShaderStageInfo.pName = "main";
    
    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
    
    // Vertex input - updated for flat float array format (8 floats per vertex) + instance data
    VkVertexInputBindingDescription bindingDescriptions[2]{};
    
    // Vertex data binding
    bindingDescriptions[0].binding = 0;
    bindingDescriptions[0].stride = 8 * sizeof(float); // 8 floats per vertex: pos(3) + normal(3) + uv(2)
    bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    // Instance data binding
    bindingDescriptions[1].binding = 1;
    bindingDescriptions[1].stride = sizeof(glm::mat4); // 4x4 matrix per instance
    bindingDescriptions[1].inputRate = VK_VERTEX_INPUT_RATE_INSTANCE;
    
    std::array<VkVertexInputAttributeDescription, 7> attributeDescriptions{};
    
    // Vertex attributes (binding 0)
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0; // Position at offset 0
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 3 * sizeof(float); // Normal at offset 12
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = 6 * sizeof(float); // UV at offset 24
    
    // Instance attributes (binding 1) - mat4 takes 4 locations
    attributeDescriptions[3].binding = 1;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[3].offset = 0; // First row of matrix
    
    attributeDescriptions[4].binding = 1;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[4].offset = 16; // Second row of matrix
    
    attributeDescriptions[5].binding = 1;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[5].offset = 32; // Third row of matrix
    
    attributeDescriptions[6].binding = 1;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = 48; // Fourth row of matrix
    
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 2;
    vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions;
    vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
    vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();
    
    // Debug: Log vertex input setup
    NOVA_INFO("CreatePipeline: Vertex input setup:");
    NOVA_INFO("  Binding 0: stride=" + std::to_string(bindingDescriptions[0].stride) + 
              ", rate=" + std::to_string(bindingDescriptions[0].inputRate));
    NOVA_INFO("  Binding 1: stride=" + std::to_string(bindingDescriptions[1].stride) + 
              ", rate=" + std::to_string(bindingDescriptions[1].inputRate));
    NOVA_INFO("  Attributes: " + std::to_string(attributeDescriptions.size()) + " total");
    for (size_t i = 0; i < attributeDescriptions.size(); ++i) {
        NOVA_INFO("    Location " + std::to_string(attributeDescriptions[i].location) + 
                  ": binding=" + std::to_string(attributeDescriptions[i].binding) + 
                  ", format=" + std::to_string(attributeDescriptions[i].format) + 
                  ", offset=" + std::to_string(attributeDescriptions[i].offset));
    }
    
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_extent.width);
    viewport.height = static_cast<float>(m_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_extent;
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = nullptr; // Dynamic viewport
    viewportState.scissorCount = 1;
    viewportState.pScissors = nullptr; // Dynamic scissor
    
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_FALSE;
    
    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    
    // Dynamic state for viewport and scissor
    VkDynamicState dynamicStates[] = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };
    
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;
    
    // Push constant range (≤256 bytes)
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstants);
    
    NOVA_INFO("CreatePipeline: Push constant size: " + std::to_string(sizeof(PushConstants)) + " bytes");
    
    NOVA_INFO("CreatePipeline: Setting up pipeline layout...");
    
    // Create descriptor set layout for uniform buffer
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &uboLayoutBinding;
    
    VK_CHECK(vkCreateDescriptorSetLayout(m_dev, &layoutInfo, nullptr, &m_descriptorSetLayout));
    
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    NOVA_INFO("CreatePipeline: About to create pipeline layout...");
    
    VkResult pipelineLayoutResult = vkCreatePipelineLayout(m_dev, &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
    if (pipelineLayoutResult != VK_SUCCESS) {
        NOVA_ERROR("Failed to create pipeline layout!");
        throw std::runtime_error("Failed to create pipeline layout");
    }
    NOVA_INFO("Pipeline layout created successfully");
    
    NOVA_INFO("CreatePipeline: About to create graphics pipeline...");
    
    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = m_pipelineLayout;
    pipelineInfo.renderPass = m_renderPass;
    pipelineInfo.subpass = 0;
    
    VkResult pipelineResult = vkCreateGraphicsPipelines(m_dev, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_pipeline);
    if (pipelineResult != VK_SUCCESS) {
        NOVA_ERROR("Failed to create graphics pipeline!");
        throw std::runtime_error("Failed to create graphics pipeline");
    }
    NOVA_INFO("Graphics pipeline created successfully");
    
    vkDestroyShaderModule(m_dev, vertShader.module, nullptr);
    vkDestroyShaderModule(m_dev, fragShader.module, nullptr);
    
    // Create descriptor pool and sets
    CreateDescriptorPool();
    CreateDescriptorSets();
}

void VulkanRenderer::CreateVertexBuffer() {
    // This function is now deprecated - vertex/index buffers are created via SetAssetData
    // Initialize with empty buffers that will be replaced by SetAssetData
    m_vertexBuffer = VK_NULL_HANDLE;
    m_vertexMemory = VK_NULL_HANDLE;
    m_indexBuffer = VK_NULL_HANDLE;
    m_indexMemory = VK_NULL_HANDLE;
    m_indexCount = 0;
    
    NOVA_INFO("VK: Vertex buffer creation deferred to SetAssetData");
}

void VulkanRenderer::CreateUniformBuffer() {
    NOVA_INFO("CreateUniformBuffer: Creating aligned uniform buffer");
    
    // Calculate aligned buffer size for proper UBO alignment
    VkDeviceSize uboSize = sizeof(UniformBufferObject);
    VkDeviceSize alignedSize = (uboSize + m_minUniformBufferOffsetAlignment - 1) & ~(m_minUniformBufferOffsetAlignment - 1);
    
    NOVA_INFO("CreateUniformBuffer: UBO size: " + std::to_string(uboSize) + ", aligned size: " + std::to_string(alignedSize));
    NOVA_INFO("CreateUniformBuffer: minUniformBufferOffsetAlignment: " + std::to_string(m_minUniformBufferOffsetAlignment));
    
    // Validate alignment
    assert(alignedSize % m_minUniformBufferOffsetAlignment == 0 && "UBO size must be aligned");
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = alignedSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(m_dev, &bufferInfo, nullptr, &m_uniformBuffer));
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_dev, m_uniformBuffer, &memRequirements);
    
    // Log memory requirements
    NOVA_INFO("CreateUniformBuffer: Memory requirements:");
    NOVA_INFO("  size: " + std::to_string(memRequirements.size));
    NOVA_INFO("  alignment: " + std::to_string(memRequirements.alignment));
    NOVA_INFO("  memoryTypeBits: " + std::to_string(memRequirements.memoryTypeBits));
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_uniformMemory));
    vkBindBufferMemory(m_dev, m_uniformBuffer, m_uniformMemory, 0);
    
    vkMapMemory(m_dev, m_uniformMemory, 0, alignedSize, 0, &m_uniformMapped);
    
    // Initialize with default values
    UniformBufferObject ubo{};
    ubo.model = glm::mat4(1.0f); // Initialize with identity matrix
    
    // Initialize light data
    for (int i = 0; i < 3; ++i) {
        ubo.lightPositions[i] = glm::vec4(0.0f, 5.0f, 0.0f, 1.0f);
        ubo.lightColors[i] = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
        ubo.lightSpaceMatrices[i] = glm::mat4(1.0f);
    }
    
    memcpy(m_uniformMapped, &ubo, sizeof(ubo));
    
    NOVA_INFO("CreateUniformBuffer: Aligned uniform buffer created successfully");
}

void VulkanRenderer::CreateDescriptorPool() {
    NOVA_INFO("CreateDescriptorPool: Creating descriptor pool");
    
    VkDescriptorPoolSize poolSize{};
    poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = &poolSize;
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;
    
    VK_CHECK(vkCreateDescriptorPool(m_dev, &poolInfo, nullptr, &m_descriptorPool));
    NOVA_INFO("CreateDescriptorPool: Descriptor pool created successfully");
}

void VulkanRenderer::CreateDescriptorSets() {
    NOVA_INFO("CreateDescriptorSets: Creating descriptor sets");
    
    // Resize descriptor sets array
    m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
    
    // Create descriptor set layouts
    std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout);
    
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
    allocInfo.pSetLayouts = layouts.data();
    
    VK_CHECK(vkAllocateDescriptorSets(m_dev, &allocInfo, m_descriptorSets.data()));
    
    // Update descriptor sets with uniform buffer
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = m_uniformBuffer;
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        VkWriteDescriptorSet descriptorWrite{};
        descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrite.dstSet = m_descriptorSets[i];
        descriptorWrite.dstBinding = 0;
        descriptorWrite.dstArrayElement = 0;
        descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrite.descriptorCount = 1;
        descriptorWrite.pBufferInfo = &bufferInfo;
        
        vkUpdateDescriptorSets(m_dev, 1, &descriptorWrite, 0, nullptr);
    }
    
    NOVA_INFO("CreateDescriptorSets: Descriptor sets created successfully");
}

void VulkanRenderer::SyncPerImageVectors(uint32_t count) {
    m_swapchainImageViews.resize(count);
    m_framebuffers.resize(count);
    m_imagesInFlight.assign(count, VK_NULL_HANDLE);
    NOVA_INFO("SyncPerImageVectors: count=" + std::to_string(count) + 
              ", views=" + std::to_string(m_swapchainImageViews.size()) + 
              ", fbs=" + std::to_string(m_framebuffers.size()) + 
              ", imagesInFlight=" + std::to_string(m_imagesInFlight.size()));
}

void VulkanRenderer::SanitySwapchainSizes() {
    assert(m_swapchainImages.size() == m_swapchainImageViews.size() && "Swapchain images and views size mismatch");
    assert(m_swapchainImages.size() == m_framebuffers.size() && "Swapchain images and framebuffers size mismatch");
    assert(m_swapchainImages.size() == m_imagesInFlight.size() && "Swapchain images and imagesInFlight size mismatch");
    NOVA_INFO("SanitySwapchainSizes: All vectors sized to " + std::to_string(m_swapchainImages.size()));
}

void VulkanRenderer::LogSwapchainSizes(const std::string& context) {
    NOVA_INFO("SwapchainSizes[" + context + "]: images=" + std::to_string(m_swapchainImages.size()) + 
              ", views=" + std::to_string(m_swapchainImageViews.size()) + 
              ", fbs=" + std::to_string(m_framebuffers.size()) + 
              ", imagesInFlight=" + std::to_string(m_imagesInFlight.size()));
}

void VulkanRenderer::CreateLightBuffer() {
    // Create light buffer for up to 8 lights (position + color for each)
    VkDeviceSize bufferSize = 8 * 2 * sizeof(glm::vec4); // 8 lights, 2 vec4s each
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(m_dev, &bufferInfo, nullptr, &m_lightBuffer));
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_dev, m_lightBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_lightMemory));
    vkBindBufferMemory(m_dev, m_lightBuffer, m_lightMemory, 0);
    
    vkMapMemory(m_dev, m_lightMemory, 0, bufferSize, 0, &m_lightMapped);
    
    // Initialize with default lights
    std::vector<glm::vec4> defaultLights = {
        glm::vec4(0.0f, 5.0f, 0.0f, 1.0f),  // Position
        glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),  // Color
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // Position
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // Color
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // Position
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // Color
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // Position
        glm::vec4(0.0f, 0.0f, 0.0f, 0.0f),  // Color
    };
    
    memcpy(m_lightMapped, defaultLights.data(), defaultLights.size() * sizeof(glm::vec4));
    m_lightCount = 1;
    
    NOVA_INFO("Light buffer created successfully");
}

void VulkanRenderer::RecreateSwapchain() {
    NOVA_INFO("RecreateSwapchain: Starting swapchain recreation");
    
    try {
        // Wait for device to be idle
        vkDeviceWaitIdle(m_dev);
        
        // Check if window is minimized
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            NOVA_INFO("RecreateSwapchain: Window minimized, waiting for resize");
            glfwWaitEvents();
            glfwGetFramebufferSize(m_window, &width, &height);
        }
        
        // Store old swapchain for proper recreation
        VkSwapchainKHR oldSwapchain = m_swapchain;
        
        // Clean up old swapchain resources
        for (auto framebuffer : m_framebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_dev, framebuffer, nullptr);
            }
        }
        m_framebuffers.clear();
        
        for (auto imageView : m_swapchainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_dev, imageView, nullptr);
            }
        }
        m_swapchainImageViews.clear();
        
        // Clean up old depth resources
        if (m_depthImageView != VK_NULL_HANDLE) {
            vkDestroyImageView(m_dev, m_depthImageView, nullptr);
            m_depthImageView = VK_NULL_HANDLE;
        }
        if (m_depthImage != VK_NULL_HANDLE) {
            vkDestroyImage(m_dev, m_depthImage, nullptr);
            m_depthImage = VK_NULL_HANDLE;
        }
        if (m_depthImageMemory != VK_NULL_HANDLE) {
            vkFreeMemory(m_dev, m_depthImageMemory, nullptr);
            m_depthImageMemory = VK_NULL_HANDLE;
        }
        
        // Clear swapchain images vector
        m_swapchainImages.clear();
        
        // Recreate swapchain (this includes creating image views)
        CreateSwapchain();
        CreateDepthResources();
        CreateFramebuffers();
        
        // Sync all per-image vectors to the new swapchain size
        SyncPerImageVectors(static_cast<uint32_t>(m_swapchainImages.size()));
        
        // Log sizes after RecreateSwapchain
        LogSwapchainSizes("After RecreateSwapchain");
        
        // Sanity check after recreation
        SanitySwapchainSizes();
        
        // Destroy old swapchain after creating new one
        if (oldSwapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_dev, oldSwapchain, nullptr);
        }
        
        NOVA_INFO("RecreateSwapchain: Swapchain recreated successfully");
        
    } catch (const std::exception& e) {
        NOVA_INFO("Error during swapchain recreation: " + std::string(e.what()));
        throw;
    }
}

void VulkanRenderer::WaitForDeviceIdle() {
    if (m_dev != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_dev);
    }
}

void VulkanRenderer::ToggleFullscreen() {
    if (!m_window) {
        NOVA_INFO("Cannot toggle fullscreen: no window available");
        return;
    }
    
    if (m_fullscreenToggleInProgress) {
        NOVA_INFO("Fullscreen toggle already in progress, ignoring");
        return;
    }
    
    m_fullscreenToggleInProgress = true;
    
    try {
        m_isFullscreen = !m_isFullscreen;
        
        if (m_isFullscreen) {
            // Get the primary monitor
            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            if (!monitor) {
                NOVA_INFO("Failed to get primary monitor");
                m_isFullscreen = false;
                m_fullscreenToggleInProgress = false;
                return;
            }
            
            const GLFWvidmode* mode = glfwGetVideoMode(monitor);
            if (!mode) {
                NOVA_INFO("Failed to get video mode");
                m_isFullscreen = false;
                m_fullscreenToggleInProgress = false;
                return;
            }
            
            // Enter fullscreen
            glfwSetWindowMonitor(m_window, monitor, 0, 0, mode->width, mode->height, mode->refreshRate);
            NOVA_INFO("Entered fullscreen mode: " + std::to_string(mode->width) + "x" + std::to_string(mode->height));
        } else {
            // Return to windowed mode
            glfwSetWindowMonitor(m_window, nullptr, 100, 100, 1280, 720, 0);
            NOVA_INFO("Returned to windowed mode: 1280x720");
        }
        
        // Let the window resize callback handle swapchain recreation
        // This is safer than forcing it immediately
        
    } catch (const std::exception& e) {
        NOVA_INFO("Exception during fullscreen toggle: " + std::string(e.what()));
        m_isFullscreen = !m_isFullscreen; // Revert the state
    }
    
    m_fullscreenToggleInProgress = false;
}

void VulkanRenderer::RenderUI(Camera* camera, LightingManager* lightingManager) {
    if (!m_imguiReady) {
        return;
    }
    

    

    
    // Main Performance Stats Window
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 300), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200), ImVec2(600, 500));
    ImGui::Begin("Vulkan Performance", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    // Engine title with gradient effect
    ImGui::TextColored(ImVec4(0.2f, 0.8f, 1.0f, 1.0f), "NovaEngine v1.0");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.8f, 0.8f, 0.8f, 0.7f), "- Real-time 3D Engine");
    ImGui::Separator();
    
    // Performance metrics with color coding
    double fps = GetFPS();
    double frameTime = GetFrameTime();
    int frameCount = GetFrameCount();
    
    // FPS with color coding
    ImVec4 fpsColor = (fps >= 55.0f) ? ImVec4(0.2f, 1.0f, 0.2f, 1.0f) : 
                     (fps >= 30.0f) ? ImVec4(1.0f, 1.0f, 0.2f, 1.0f) : 
                     ImVec4(1.0f, 0.2f, 0.2f, 1.0f);
    
    ImGui::TextColored(ImVec4(1.0f, 1.0f, 1.0f, 1.0f), "Performance:");
    ImGui::TextColored(fpsColor, "FPS: %.1f", fps);
    ImGui::Text("Frame Time: %.2f ms", frameTime);
    ImGui::Text("Frame Count: %d", frameCount);
    
    ImGui::Separator();
    
    // Camera info with detailed stats
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Camera System:");
    if (camera) {
        glm::vec3 pos = camera->GetPosition();
        float fov = camera->GetFOV();
        ImGui::Text("Position: (%.2f, %.2f, %.2f)", pos.x, pos.y, pos.z);
        ImGui::Text("FOV: %.1f", fov);
        
        // Interactive movement speed control
        float movementSpeed = camera->GetMovementSpeed();
        if (ImGui::SliderFloat("Movement Speed", &movementSpeed, 0.1f, 20.0f, "%.1f")) {
            camera->SetMovementSpeed(movementSpeed);
        }
    } else {
        ImGui::Text("Position: (0.00, 0.00, 8.00)");
        ImGui::Text("FOV: 45.0");
        ImGui::Text("Movement Speed: 5.0");
    }
    
    ImGui::Separator();
    
    // Lighting info with real-time updates
    ImGui::TextColored(ImVec4(1.0f, 0.6f, 0.2f, 1.0f), "Lighting System:");
    ImGui::Text("Active Lights: 3");
    
    // Interactive light controls
    static float light1Pos[3] = {-5.97f, 3.99f, -5.33f};
    static float light1Intensity = 1.00f;
    static float light2Pos[3] = {-4.00f, 2.00f, -3.00f};
    static float light2Intensity = 0.03f;
    static float light3Pos[3] = {-2.28f, 2.00f, -5.55f};
    static float light3Intensity = 0.60f;
    static bool lightsInitialized = false;
    
    // Initialize with actual light data if available (only once)
    if (!lightsInitialized && lightingManager && lightingManager->GetLightCount() >= 3) {
        const auto& lights = lightingManager->GetLights();
        light1Pos[0] = lights[0].position.x; light1Pos[1] = lights[0].position.y; light1Pos[2] = lights[0].position.z;
        light1Intensity = lights[0].intensity;
        light2Pos[0] = lights[1].position.x; light2Pos[1] = lights[1].position.y; light2Pos[2] = lights[1].position.z;
        light2Intensity = lights[1].intensity;
        light3Pos[0] = lights[2].position.x; light3Pos[1] = lights[2].position.y; light3Pos[2] = lights[2].position.z;
        light3Intensity = lights[2].intensity;
        lightsInitialized = true;
    }
    
    ImGui::Text("Light 1:");
    ImGui::SameLine();
    ImGui::Text("Pos(%.2f, %.2f, %.2f) Intensity: %.2f", light1Pos[0], light1Pos[1], light1Pos[2], light1Intensity);
    if (ImGui::InputFloat3("Light 1 Position", light1Pos, "%.2f")) {
        UpdateLightInManager(0, glm::vec3(light1Pos[0], light1Pos[1], light1Pos[2]), light1Intensity, lightingManager);
    }
    if (ImGui::InputFloat("Light 1 Intensity", &light1Intensity, 0.01f, 0.1f, "%.2f")) {
        UpdateLightInManager(0, glm::vec3(light1Pos[0], light1Pos[1], light1Pos[2]), light1Intensity, lightingManager);
    }
    
    ImGui::Text("Light 2:");
    ImGui::SameLine();
    ImGui::Text("Pos(%.2f, %.2f, %.2f) Intensity: %.2f", light2Pos[0], light2Pos[1], light2Pos[2], light2Intensity);
    if (ImGui::InputFloat3("Light 2 Position", light2Pos, "%.2f")) {
        UpdateLightInManager(1, glm::vec3(light2Pos[0], light2Pos[1], light2Pos[2]), light2Intensity, lightingManager);
    }
    if (ImGui::InputFloat("Light 2 Intensity", &light2Intensity, 0.01f, 0.1f, "%.2f")) {
        UpdateLightInManager(1, glm::vec3(light2Pos[0], light2Pos[1], light2Pos[2]), light2Intensity, lightingManager);
    }
    
    ImGui::Text("Light 3:");
    ImGui::SameLine();
    ImGui::Text("Pos(%.2f, %.2f, %.2f) Intensity: %.2f", light3Pos[0], light3Pos[1], light3Pos[2], light3Intensity);
    if (ImGui::InputFloat3("Light 3 Position", light3Pos, "%.2f")) {
        UpdateLightInManager(2, glm::vec3(light3Pos[0], light3Pos[1], light3Pos[2]), light3Intensity, lightingManager);
    }
    if (ImGui::InputFloat("Light 3 Intensity", &light3Intensity, 0.01f, 0.1f, "%.2f")) {
        UpdateLightInManager(2, glm::vec3(light3Pos[0], light3Pos[1], light3Pos[2]), light3Intensity, lightingManager);
    }
    
    ImGui::End();
    
    // Controls Window with enhanced functionality
    ImGui::SetNextWindowPos(ImVec2(20, 340), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 350), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 250), ImVec2(600, 500));
    ImGui::Begin("Vulkan Controls", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    ImGui::TextColored(ImVec4(0.8f, 1.0f, 0.2f, 1.0f), "Camera Controls:");
    ImGui::Text("WASD - Move Camera");
    ImGui::Text("Mouse - Look Around");
    ImGui::Text("Scroll - Zoom In/Out");
    ImGui::Text("ESC or Q - Exit");
    ImGui::Text("F11 - Toggle Fullscreen");
    ImGui::Text("All UI windows can be moved by dragging their title bars");
    
    ImGui::Separator();
    
    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "Lighting Presets:");
    if (ImGui::Button("Default Lighting (1)", ImVec2(140, 25))) {
        // TODO: Implement lighting preset switching
    }
    ImGui::SameLine();
    if (ImGui::Button("Three-Point (2)", ImVec2(140, 25))) {
        // TODO: Implement lighting preset switching
    }
    if (ImGui::Button("Dramatic Lighting (3)", ImVec2(140, 25))) {
        // TODO: Implement lighting preset switching
    }
    ImGui::SameLine();
    if (ImGui::Button("Reset Camera", ImVec2(140, 25))) {
        // TODO: Implement camera reset
    }
    ImGui::SameLine();
    if (ImGui::Button("Toggle Fullscreen (F11)", ImVec2(140, 25))) {
        ToggleFullscreen();
    }
    
    ImGui::End();
    
    // Scene Info Window with technical details
    ImGui::SetNextWindowPos(ImVec2(20, 710), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(380, 250), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200), ImVec2(600, 400));
    ImGui::Begin("Vulkan Scene Info", nullptr, ImGuiWindowFlags_AlwaysVerticalScrollbar);
    
    ImGui::TextColored(ImVec4(0.8f, 0.2f, 1.0f, 1.0f), "Rendering System:");
    ImGui::Text("Spheres Rendered: 27");
    ImGui::Text("GPU Instancing: Enabled");
    ImGui::Text("PBR Materials: Active");
    ImGui::Text("Vulkan API: Active");
    ImGui::Text("Asset Hot-Reload: Active");
    
    ImGui::End();
}

void VulkanRenderer::CreateCommandPool() {
    NOVA_INFO("CreateCommandPool: Creating command pool for frame-in-flight rendering");
    
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = m_queueFamily;
    
    VK_CHECK(vkCreateCommandPool(m_dev, &poolInfo, nullptr, &m_cmdPool));
    
    // Resize command buffer array first
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    NOVA_INFO("CreateCommandPool: Resized command buffer array to " + std::to_string(MAX_FRAMES_IN_FLIGHT) + " elements");
    
    // Validate allocation parameters
    assert(MAX_FRAMES_IN_FLIGHT > 0 && "Command buffer count must be greater than 0");
    assert(m_commandBuffers.data() != nullptr && "Command buffer array must not be null");
    
    // Allocate command buffers for each frame-in-flight
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());
    
    NOVA_INFO("CreateCommandPool: Allocating " + std::to_string(allocInfo.commandBufferCount) + " command buffers");
    VK_CHECK(vkAllocateCommandBuffers(m_dev, &allocInfo, m_commandBuffers.data()));
    
    // Set debug names for command buffers
#ifdef _DEBUG
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        SetDebugName(m_commandBuffers[i], "CommandBuffer_" + std::to_string(i));
    }
#endif
    
    // Keep the old single command buffer for compatibility (will be removed later)
    m_cmdBuffer = m_commandBuffers[0];
    
    NOVA_INFO("CreateCommandPool: " + std::to_string(MAX_FRAMES_IN_FLIGHT) + " command buffers allocated successfully");
}

void VulkanRenderer::CreateSyncObjects() {
    NOVA_INFO("CreateSyncObjects: Creating frame-in-flight synchronization objects");
    
    // Resize arrays to MAX_FRAMES_IN_FLIGHT
    m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
    m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled so first frame doesn't wait
    
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    
    // Create synchronization objects for each frame-in-flight
    for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        NOVA_INFO("CreateSyncObjects: Creating sync objects for frame " + std::to_string(i));
        
        VK_CHECK(vkCreateSemaphore(m_dev, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]));
        VK_CHECK(vkCreateSemaphore(m_dev, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]));
        VK_CHECK(vkCreateFence(m_dev, &fenceInfo, nullptr, &m_inFlightFences[i]));
        
        // Set debug names for validation
#ifdef _DEBUG
        SetDebugName(m_imageAvailableSemaphores[i], "ImageAvailableSemaphore_" + std::to_string(i));
        SetDebugName(m_renderFinishedSemaphores[i], "RenderFinishedSemaphore_" + std::to_string(i));
        SetDebugName(m_inFlightFences[i], "InFlightFence_" + std::to_string(i));
#endif
    }
    
    // Initialize imagesInFlight array (will be resized when swapchain is created)
    m_imagesInFlight.resize(0);
    
    NOVA_INFO("CreateSyncObjects: Frame-in-flight sync objects created successfully");
}

uint32_t VulkanRenderer::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_phys, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

void VulkanRenderer::RecordCommandBuffer(VkCommandBuffer cmd, VkFramebuffer framebuffer, Camera* camera) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    
    VK_CHECK(vkBeginCommandBuffer(cmd, &beginInfo));
    
    // Render shadow maps using the new shadow system
    // For now, just a placeholder to prevent crashes
    // TODO: Implement proper shadow rendering with the new system
    
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = m_renderPass;
    renderPassInfo.framebuffer = framebuffer;
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = m_extent;
    
    VkClearValue clearValues[2];
    clearValues[0].color = {{0.2f, 0.3f, 0.4f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearValues;
    
    vkCmdBeginRenderPass(cmd, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    
    // Track current render pass
    m_currentRenderPass = m_renderPass;
    
    // Set viewport dynamically to match current extent
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(m_extent.width);
    viewport.height = static_cast<float>(m_extent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(cmd, 0, 1, &viewport);
    
    // Set scissor dynamically to match current extent
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = m_extent;
    vkCmdSetScissor(cmd, 0, 1, &scissor);
    
    vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipeline);
    
    // Bind descriptor set for uniform buffer
    vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &m_descriptorSets[m_currentFrame], 0, nullptr);
    
    // Bind vertex buffer (binding 0)
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(cmd, 0, 1, &m_vertexBuffer, offsets);
    
    // Bind instance buffer if we have instances (binding 1)
    if (m_instanceBuffer != VK_NULL_HANDLE && m_instanceCount > 0) {
        vkCmdBindVertexBuffers(cmd, 1, 1, &m_instanceBuffer, offsets);
    }
    
    vkCmdBindIndexBuffer(cmd, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    // Use the current uniform buffer data (updated by UpdateMVP)
    UniformBufferObject ubo{};
    memcpy(&ubo, m_uniformMapped, sizeof(ubo));
    
    // Create push constants with view-projection matrix and material data
    PushConstants pushConstants{};
    
    // Use camera if provided, otherwise use default view
    glm::mat4 view;
    glm::mat4 projection;
    float aspectRatio = static_cast<float>(m_extent.width) / static_cast<float>(m_extent.height);
    
    if (camera) {
        // Use camera's view and projection matrices
        view = camera->GetViewMatrix();
        projection = camera->GetProjectionMatrix();
        
        // Debug: Log camera position
        glm::vec3 camPos = camera->GetPosition();
        NOVA_INFO("RecordCommandBuffer: Using camera at (" + std::to_string(camPos.x) + 
                  ", " + std::to_string(camPos.y) + ", " + std::to_string(camPos.z) + 
                  "), aspect ratio: " + std::to_string(aspectRatio) + 
                  ", extent: " + std::to_string(m_extent.width) + "x" + std::to_string(m_extent.height));
    } else {
        // Fallback to default view
        view = glm::lookAt(glm::vec3(6.0f, 4.0f, 6.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        projection = glm::perspective(glm::radians(45.0f), aspectRatio, 0.1f, 100.0f);
        
        NOVA_INFO("RecordCommandBuffer: Using default camera at (6,4,6), aspect ratio: " + std::to_string(aspectRatio) + 
                  ", extent: " + std::to_string(m_extent.width) + "x" + std::to_string(m_extent.height));
    }
    
    pushConstants.viewProjection = projection * view;
    pushConstants.baseColor = glm::vec4(1.0f, 0.2f, 0.2f, 1.0f); // Bright red color
    pushConstants.metallic = 0.0f;
    pushConstants.roughness = 0.3f;
    
    NOVA_INFO("RecordCommandBuffer: Pushing constants, size: " + std::to_string(sizeof(PushConstants)) + " bytes");
    vkCmdPushConstants(cmd, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(PushConstants), &pushConstants);
    
    // Draw with instancing if we have instances, otherwise draw normally
    NOVA_INFO("RecordCommandBuffer: About to draw indexed");
    if (m_instanceBuffer != VK_NULL_HANDLE && m_instanceCount > 0) {
        NOVA_INFO("RecordCommandBuffer: Drawing " + std::to_string(m_indexCount) + " indices with " + std::to_string(m_instanceCount) + " instances");
        vkCmdDrawIndexed(cmd, m_indexCount, m_instanceCount, 0, 0, 0);
        NOVA_INFO("RecordCommandBuffer: Instanced draw completed");
    } else {
        NOVA_INFO("RecordCommandBuffer: Drawing " + std::to_string(m_indexCount) + " indices with 1 instance");
        vkCmdDrawIndexed(cmd, m_indexCount, 1, 0, 0, 0);
        NOVA_INFO("RecordCommandBuffer: Single draw completed");
    }
    
    // Render ImGui UI within the render pass
    NOVA_INFO("RecordCommandBuffer: About to render ImGui UI");
    if (m_imguiReady) {
        ImDrawData* drawData = ImGui::GetDrawData();

        if (drawData && drawData->Valid) {
            NOVA_INFO("RecordCommandBuffer: Rendering ImGui draw data");
            ImGui_ImplVulkan_RenderDrawData(drawData, cmd);
            NOVA_INFO("RecordCommandBuffer: ImGui draw data rendered");
        } else {
            NOVA_INFO("RecordCommandBuffer: ImGui draw data not valid");
        }
    } else {
        NOVA_INFO("RecordCommandBuffer: ImGui not ready");
    }
        
    NOVA_INFO("RecordCommandBuffer: Ending render pass");
    vkCmdEndRenderPass(cmd);
    NOVA_INFO("RecordCommandBuffer: Render pass ended");
    
    VK_CHECK(vkEndCommandBuffer(cmd));
}

void VulkanRenderer::RenderFrame(Camera* camera, LightingManager* lightingManager) {
    // Declare all variables that might be used after goto before any goto paths
    uint32_t imageIndex;
    VkResult fenceResult;
    VkResult result;
    VkResult waitResult;
    VkResult resetResult;
    VkResult resetCmdResult;
    VkSubmitInfo submitInfo{};
    VkSemaphore waitSemaphores[1];
    VkPipelineStageFlags waitStages[1];
    VkSemaphore signalSemaphores[1];
    VkResult submitResult;
    VkPresentInfoKHR presentInfo{};
    VkSwapchainKHR swapChains[1];
    
    NOVA_INFO("RenderFrame: Starting frame render - Frame " + std::to_string(m_currentFrame));
    
    // Log current frame and swapchain sizes for debugging
    NOVA_INFO("RenderFrame: currentFrame=" + std::to_string(m_currentFrame) + " (MAX_FRAMES_IN_FLIGHT=" + std::to_string(MAX_FRAMES_IN_FLIGHT) + ")");
    NOVA_INFO("RenderFrame: swapchainImageCount=" + std::to_string(m_swapchainImages.size()));
    
    // Begin ImGui frame
    BeginFrame();
    NOVA_INFO("RenderFrame: ImGui frame begun");
    
    // UI rendering with camera and lighting data
    NOVA_INFO("RenderFrame: About to render UI");
    RenderUI(camera, lightingManager);
    NOVA_INFO("RenderFrame: UI rendered");
    
    // Log sizes before frame processing
    LogSwapchainSizes("Before frame processing");
    
    // Auto-heal size mismatches at runtime instead of aborting
    if (m_swapchainImages.size() != m_imagesInFlight.size()) {
        NOVA_ERROR("Per-image arrays out of sync at frame start; healing (images=" + std::to_string(m_swapchainImages.size()) + 
                   ", imagesInFlight=" + std::to_string(m_imagesInFlight.size()) + ")");
        SyncPerImageVectors(static_cast<uint32_t>(m_swapchainImages.size()));
    }
    
    // If healing didn't work, recreate swapchain and skip this frame
    if (m_swapchainImages.size() != m_imagesInFlight.size()) {
        NOVA_ERROR("Per-image arrays still out of sync after healing; recreating swapchain.");
        RecreateSwapchain();
        goto FrameCleanup; // Skip this frame cleanly
    }
    
    // Validate array sizes - assert once per frame that sizes match (after healing)
    assert(m_swapchainImages.size() == m_imagesInFlight.size() && "ImagesInFlight size mismatch after heal");
    assert(m_swapchainImages.size() == m_framebuffers.size() && "Framebuffers size mismatch");
    assert(m_commandBuffers.size() == MAX_FRAMES_IN_FLIGHT && "Command buffers size mismatch");
    
    // Wait for the fence for the current frame to be signaled
    NOVA_INFO("RenderFrame: Waiting for fence " + std::to_string(m_currentFrame));
    fenceResult = vkWaitForFences(m_dev, 1, &idx(m_inFlightFences, m_currentFrame, "inFlightFences"), VK_TRUE, UINT64_MAX);
    if (fenceResult != VK_SUCCESS) {
        NOVA_ERROR("RenderFrame: Failed to wait for fence: " + std::to_string(fenceResult));
        goto FrameCleanup;
    }
    NOVA_INFO("RenderFrame: Fence " + std::to_string(m_currentFrame) + " waited successfully");
    
    // Acquire the next image from the swapchain
    NOVA_INFO("RenderFrame: About to acquire next image");
    result = vkAcquireNextImageKHR(m_dev, m_swapchain, UINT64_MAX, 
                                           idx(m_imageAvailableSemaphores, m_currentFrame, "imageAvailableSemaphores"), VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        NOVA_INFO("RenderFrame: Swapchain out of date during acquire, recreating...");
        RecreateSwapchain();
        goto FrameCleanup;
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        NOVA_ERROR("RenderFrame: Failed to acquire next image: " + std::to_string(result));
        goto FrameCleanup;
    }
    NOVA_INFO("RenderFrame: Image " + std::to_string(imageIndex) + " acquired successfully");
    
    // Log image index bounds check
    NOVA_INFO("RenderFrame: imageIndex=" + std::to_string(imageIndex) + " (swapchainImageCount=" + std::to_string(m_swapchainImages.size()) + ")");
    
    // Hard guards against imageIndex out of range
    if (imageIndex >= m_swapchainImages.size() || imageIndex >= m_imagesInFlight.size()) {
        NOVA_ERROR("RenderFrame: imageIndex " + std::to_string(imageIndex) + " out of range (images=" + std::to_string(m_swapchainImages.size()) + ", imagesInFlight=" + std::to_string(m_imagesInFlight.size()) + ")");
        goto FrameCleanup;
    }
    
    // Check if a previous frame is using this image (i.e. there is its fence to wait on)
    if (m_imagesInFlight[imageIndex] != VK_NULL_HANDLE) {
        NOVA_INFO("RenderFrame: Waiting for previous frame to finish using image " + std::to_string(imageIndex));
        waitResult = vkWaitForFences(m_dev, 1, &m_imagesInFlight[imageIndex], VK_TRUE, UINT64_MAX);
        if (waitResult != VK_SUCCESS) {
            NOVA_ERROR("RenderFrame: Failed to wait for previous frame fence: " + std::to_string(waitResult));
            goto FrameCleanup;
        }
    }
    
    // Mark the image as now being in use by this frame's fence
    m_imagesInFlight[imageIndex] = idx(m_inFlightFences, m_currentFrame, "inFlightFences");
    
    // Reset the fence for the current frame
    NOVA_INFO("RenderFrame: Resetting fence " + std::to_string(m_currentFrame));
    resetResult = vkResetFences(m_dev, 1, &idx(m_inFlightFences, m_currentFrame, "inFlightFences"));
    if (resetResult != VK_SUCCESS) {
        NOVA_ERROR("RenderFrame: Failed to reset fence: " + std::to_string(resetResult));
        goto FrameCleanup;
    }
    
    // End ImGui frame BEFORE recording command buffer
    if (m_imguiReady) {
        ImGui::Render();
    }
    
    // Reset and record the command buffer for the current frame
    NOVA_INFO("RenderFrame: Resetting command buffer " + std::to_string(m_currentFrame));
    resetCmdResult = vkResetCommandBuffer(idx(m_commandBuffers, m_currentFrame, "commandBuffers"), 0);
    if (resetCmdResult != VK_SUCCESS) {
        NOVA_ERROR("RenderFrame: Failed to reset command buffer: " + std::to_string(resetCmdResult));
        goto FrameCleanup;
    }
    
    NOVA_INFO("RenderFrame: Recording command buffer " + std::to_string(m_currentFrame) + " for image " + std::to_string(imageIndex));
    RecordCommandBuffer(idx(m_commandBuffers, m_currentFrame, "commandBuffers"), idx(m_framebuffers, imageIndex, "framebuffers"), camera);
    
    // Submit the command buffer
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    waitSemaphores[0] = idx(m_imageAvailableSemaphores, m_currentFrame, "imageAvailableSemaphores");
    waitStages[0] = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &idx(m_commandBuffers, m_currentFrame, "commandBuffers");
    
    signalSemaphores[0] = idx(m_renderFinishedSemaphores, m_currentFrame, "renderFinishedSemaphores");
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    NOVA_INFO("RenderFrame: Submitting command buffer " + std::to_string(m_currentFrame) + " with fence " + std::to_string(m_currentFrame));
    submitResult = vkQueueSubmit(m_queue, 1, &submitInfo, idx(m_inFlightFences, m_currentFrame, "inFlightFences"));
    if (submitResult != VK_SUCCESS) {
        NOVA_ERROR("RenderFrame: Failed to submit command buffer: " + std::to_string(submitResult));
        goto FrameCleanup;
    }
    NOVA_INFO("RenderFrame: Command buffer submitted successfully");
    
    // Present the image
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    swapChains[0] = m_swapchain;
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    NOVA_INFO("RenderFrame: About to present image " + std::to_string(imageIndex));
    result = vkQueuePresentKHR(m_queue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        NOVA_INFO("RenderFrame: Swapchain out of date or suboptimal during present, will recreate next frame");
        // Don't return, just note that we need to recreate the swapchain
    } else if (result != VK_SUCCESS) {
        NOVA_ERROR("RenderFrame: Failed to present image: " + std::to_string(result));
        goto FrameCleanup;
    }
    NOVA_INFO("RenderFrame: Image presented successfully");
    
    // Advance to the next frame only after successful present
    m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    NOVA_INFO("RenderFrame: Advanced to frame " + std::to_string(m_currentFrame));
    
    NOVA_INFO("RenderFrame: Frame completed successfully");
    NOVA_INFO("RenderFrame: SUCCESSFULLY RETURNING FROM RenderFrame");
    return;

FrameCleanup:
    // Always end ImGui frame and render, even on errors
    NOVA_INFO("RenderFrame: Entering FrameCleanup");
    if (m_imguiReady) {
        ImGui::EndFrame();
        ImGui::Render();
        NOVA_INFO("RenderFrame: ImGui frame ended and rendered in cleanup");
    }
    NOVA_INFO("RenderFrame: Frame cleanup completed");
}

void VulkanRenderer::UpdateMVP(const glm::mat4& mvp) {
    m_currentMVP = mvp; // Store the current MVP matrix
    
    UniformBufferObject ubo{};
    ubo.model = mvp; // Store the model matrix in UBO
    
    // Copy light data from stored arrays (this ensures lights are updated every frame)
    for (int i = 0; i < 3; ++i) {
        if (i < static_cast<int>(m_lightPositions.size())) {
            ubo.lightPositions[i] = m_lightPositions[i];
            ubo.lightColors[i] = m_lightColors[i];
        } else {
            ubo.lightPositions[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
            ubo.lightColors[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
        }
    }
    
    // Calculate light space matrices for all lights (for shadow mapping)
    // TODO: Implement proper light space matrix calculation with the new shadow system
    for (int i = 0; i < 3; i++) {
        ubo.lightSpaceMatrices[i] = glm::mat4(1.0f);
    }
    
    memcpy(m_uniformMapped, &ubo, sizeof(ubo));
}

void VulkanRenderer::UpdateMVP(float deltaTime) {
    // Create a simple rotation animation
    static float rotation = 0.0f;
    rotation += deltaTime * 90.0f; // 90 degrees per second
    
    glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(rotation), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 projection = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    
    glm::mat4 mvp = projection * view * model;
    UpdateMVP(mvp);
}

void VulkanRenderer::SetAssetData(const std::vector<float>& vertexData, const std::vector<uint32_t>& indices) {
    NOVA_INFO("SetAssetData: Creating device-local buffers with staging");
    
    // Destroy existing buffers if they exist
    if (m_vertexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_dev, m_vertexBuffer, nullptr);
        vkFreeMemory(m_dev, m_vertexMemory, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
        m_vertexMemory = VK_NULL_HANDLE;
    }
    
    if (m_indexBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_dev, m_indexBuffer, nullptr);
        vkFreeMemory(m_dev, m_indexMemory, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
        m_indexMemory = VK_NULL_HANDLE;
    }
    
    // Create vertex buffer with proper staging
    VkDeviceSize vertexBufferSize = vertexData.size() * sizeof(float);
    NOVA_INFO("SetAssetData: Creating vertex buffer of size " + std::to_string(vertexBufferSize) + " bytes");
    
    // Create staging buffer
    VkBuffer stagingVertexBuffer;
    VkDeviceMemory stagingVertexMemory;
    
    VkBufferCreateInfo stagingBufferInfo{};
    stagingBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    stagingBufferInfo.size = vertexBufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    stagingBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(m_dev, &stagingBufferInfo, nullptr, &stagingVertexBuffer));
    
    VkMemoryRequirements stagingMemRequirements;
    vkGetBufferMemoryRequirements(m_dev, stagingVertexBuffer, &stagingMemRequirements);
    
    VkMemoryAllocateInfo stagingAllocInfo{};
    stagingAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    stagingAllocInfo.allocationSize = stagingMemRequirements.size;
    stagingAllocInfo.memoryTypeIndex = FindMemoryType(stagingMemRequirements.memoryTypeBits, 
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &stagingAllocInfo, nullptr, &stagingVertexMemory));
    vkBindBufferMemory(m_dev, stagingVertexBuffer, stagingVertexMemory, 0);
    
    // Copy data to staging buffer
    void* stagingData;
    vkMapMemory(m_dev, stagingVertexMemory, 0, vertexBufferSize, 0, &stagingData);
    memcpy(stagingData, vertexData.data(), vertexBufferSize);
    vkUnmapMemory(m_dev, stagingVertexMemory);
    
    // Create device-local vertex buffer
    VkBufferCreateInfo vertexBufferInfo{};
    vertexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    vertexBufferInfo.size = vertexBufferSize;
    vertexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    vertexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(m_dev, &vertexBufferInfo, nullptr, &m_vertexBuffer));
    
    VkMemoryRequirements vertexMemRequirements;
    vkGetBufferMemoryRequirements(m_dev, m_vertexBuffer, &vertexMemRequirements);
    
    VkMemoryAllocateInfo vertexAllocInfo{};
    vertexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    vertexAllocInfo.allocationSize = vertexMemRequirements.size;
    vertexAllocInfo.memoryTypeIndex = FindMemoryType(vertexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &vertexAllocInfo, nullptr, &m_vertexMemory));
    vkBindBufferMemory(m_dev, m_vertexBuffer, m_vertexMemory, 0);
    
    // Create index buffer with proper staging
    VkDeviceSize indexBufferSize = indices.size() * sizeof(uint32_t);
    m_indexCount = static_cast<uint32_t>(indices.size());
    NOVA_INFO("SetAssetData: Creating index buffer of size " + std::to_string(indexBufferSize) + " bytes");
    
    // Create staging buffer for indices
    VkBuffer stagingIndexBuffer;
    VkDeviceMemory stagingIndexMemory;
    
    stagingBufferInfo.size = indexBufferSize;
    stagingBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    
    VK_CHECK(vkCreateBuffer(m_dev, &stagingBufferInfo, nullptr, &stagingIndexBuffer));
    
    vkGetBufferMemoryRequirements(m_dev, stagingIndexBuffer, &stagingMemRequirements);
    
    stagingAllocInfo.allocationSize = stagingMemRequirements.size;
    stagingAllocInfo.memoryTypeIndex = FindMemoryType(stagingMemRequirements.memoryTypeBits, 
                                                      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &stagingAllocInfo, nullptr, &stagingIndexMemory));
    vkBindBufferMemory(m_dev, stagingIndexBuffer, stagingIndexMemory, 0);
    
    // Copy index data to staging buffer
    vkMapMemory(m_dev, stagingIndexMemory, 0, indexBufferSize, 0, &stagingData);
    memcpy(stagingData, indices.data(), indexBufferSize);
    vkUnmapMemory(m_dev, stagingIndexMemory);
    
    // Create device-local index buffer
    VkBufferCreateInfo indexBufferInfo{};
    indexBufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    indexBufferInfo.size = indexBufferSize;
    indexBufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
    indexBufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(m_dev, &indexBufferInfo, nullptr, &m_indexBuffer));
    
    VkMemoryRequirements indexMemRequirements;
    vkGetBufferMemoryRequirements(m_dev, m_indexBuffer, &indexMemRequirements);
    
    VkMemoryAllocateInfo indexAllocInfo{};
    indexAllocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    indexAllocInfo.allocationSize = indexMemRequirements.size;
    indexAllocInfo.memoryTypeIndex = FindMemoryType(indexMemRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &indexAllocInfo, nullptr, &m_indexMemory));
    vkBindBufferMemory(m_dev, m_indexBuffer, m_indexMemory, 0);
    
    // Copy from staging to device-local buffers using command buffer
    VkCommandBuffer copyCmd = BeginSingleTimeCommands();
    
    VkBufferCopy vertexCopyRegion{};
    vertexCopyRegion.srcOffset = 0;
    vertexCopyRegion.dstOffset = 0;
    vertexCopyRegion.size = vertexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingVertexBuffer, m_vertexBuffer, 1, &vertexCopyRegion);
    
    VkBufferCopy indexCopyRegion{};
    indexCopyRegion.srcOffset = 0;
    indexCopyRegion.dstOffset = 0;
    indexCopyRegion.size = indexBufferSize;
    vkCmdCopyBuffer(copyCmd, stagingIndexBuffer, m_indexBuffer, 1, &indexCopyRegion);
    
    EndSingleTimeCommands(copyCmd);
    
    // Clean up staging buffers
    vkDestroyBuffer(m_dev, stagingVertexBuffer, nullptr);
    vkFreeMemory(m_dev, stagingVertexMemory, nullptr);
    vkDestroyBuffer(m_dev, stagingIndexBuffer, nullptr);
    vkFreeMemory(m_dev, stagingIndexMemory, nullptr);
    
    NOVA_INFO("SetAssetData: Device-local buffers created successfully");
    NOVA_INFO("Asset data set: " + std::to_string(vertexData.size() / 8) + " vertices, " + std::to_string(indices.size()) + " indices");
    NOVA_INFO("First few vertices: ");
    for (int i = 0; i < std::min(static_cast<int>(vertexData.size()), 24); i += 8) {
        NOVA_INFO("  V" + std::to_string(i/8) + ": pos(" + 
                 std::to_string(vertexData[i]) + ", " + 
                 std::to_string(vertexData[i+1]) + ", " + 
                 std::to_string(vertexData[i+2]) + ")");
    }
}

void VulkanRenderer::SetInstanceData(const std::vector<glm::mat4>& instanceMatrices) {
    // Destroy existing instance buffer if it exists
    if (m_instanceBuffer != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_dev, m_instanceBuffer, nullptr);
        vkFreeMemory(m_dev, m_instanceMemory, nullptr);
        m_instanceBuffer = VK_NULL_HANDLE;
        m_instanceMemory = VK_NULL_HANDLE;
    }
    
    if (instanceMatrices.empty()) {
        m_instanceCount = 0;
        return;
    }
    
    // Create new instance buffer
    VkDeviceSize bufferSize = instanceMatrices.size() * sizeof(glm::mat4);
    m_instanceCount = static_cast<uint32_t>(instanceMatrices.size());
    
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = bufferSize;
    bufferInfo.usage = VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    
    VK_CHECK(vkCreateBuffer(m_dev, &bufferInfo, nullptr, &m_instanceBuffer));
    
    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(m_dev, m_instanceBuffer, &memRequirements);
    
    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
    
    VK_CHECK(vkAllocateMemory(m_dev, &allocInfo, nullptr, &m_instanceMemory));
    vkBindBufferMemory(m_dev, m_instanceBuffer, m_instanceMemory, 0);
    
    void* data;
    vkMapMemory(m_dev, m_instanceMemory, 0, bufferSize, 0, &data);
    memcpy(data, instanceMatrices.data(), bufferSize);
    vkUnmapMemory(m_dev, m_instanceMemory);
    
    // Debug: Log the first instance matrix to verify translation is in the right place
    if (!instanceMatrices.empty()) {
        const glm::mat4& firstMatrix = instanceMatrices[0];
        glm::vec3 translation = glm::vec3(firstMatrix[3]); // GLM is column-major, so translation is in column 3
        NOVA_INFO("First instance matrix translation: (" + std::to_string(translation.x) + 
                  ", " + std::to_string(translation.y) + ", " + std::to_string(translation.z) + ")");
        
        // Also log the matrix structure to verify it's correct
        NOVA_INFO("First instance matrix structure:");
        for (int i = 0; i < 4; ++i) {
            NOVA_INFO("  Row " + std::to_string(i) + ": (" + 
                      std::to_string(firstMatrix[i][0]) + ", " + std::to_string(firstMatrix[i][1]) + 
                      ", " + std::to_string(firstMatrix[i][2]) + ", " + std::to_string(firstMatrix[i][3]) + ")");
        }
    }
    
    NOVA_INFO("Instance data set: " + std::to_string(m_instanceCount) + " instances");
}

void VulkanRenderer::SetLights(const std::vector<glm::vec4>& lightPositions, const std::vector<glm::vec4>& lightColors) {
    if (lightPositions.empty() || lightColors.empty()) {
        m_lightCount = 0;
        return;
    }
    
    // Ensure we have matching arrays
    size_t count = std::min(lightPositions.size(), lightColors.size());
    m_lightCount = static_cast<uint32_t>(count);
    
    // Create light data array
    std::vector<glm::vec4> lightData;
    lightData.reserve(count * 2); // positions + colors
    
    for (size_t i = 0; i < count; ++i) {
        lightData.push_back(lightPositions[i]);
        lightData.push_back(lightColors[i]);
    }
    
    // Update light buffer
    if (m_lightMapped) {
        memcpy(m_lightMapped, lightData.data(), lightData.size() * sizeof(glm::vec4));
    }
    
    NOVA_INFO("Light data set: " + std::to_string(m_lightCount) + " lights");
    
    // Store the light data for UI updates
    m_lightPositions = lightPositions;
    m_lightColors = lightColors;
}

void VulkanRenderer::UpdateLight(int lightIndex, const glm::vec3& position, float intensity) {
    if (lightIndex < 0 || lightIndex >= static_cast<int>(m_lightCount)) {
        return;
    }
    
    // Update position (w component is unused for point lights)
    m_lightPositions[lightIndex] = glm::vec4(position, 1.0f);
    
    // Update color/intensity (assuming white light with intensity)
    m_lightColors[lightIndex] = glm::vec4(intensity, intensity, intensity, 1.0f);
    
    // Update the light buffer
    std::vector<glm::vec4> lightData;
    lightData.reserve(m_lightCount * 2);
    
    for (size_t i = 0; i < m_lightCount; ++i) {
        lightData.push_back(m_lightPositions[i]);
        lightData.push_back(m_lightColors[i]);
    }
    
    if (m_lightMapped) {
        memcpy(m_lightMapped, lightData.data(), lightData.size() * sizeof(glm::vec4));
    }
}

void VulkanRenderer::UpdateLightInManager(int lightIndex, const glm::vec3& position, float intensity, LightingManager* lightingManager) {
    if (!lightingManager || lightIndex < 0 || lightIndex >= static_cast<int>(lightingManager->GetLightCount())) {
        return;
    }
    
    // Get the current lights
    auto lights = lightingManager->GetLights();
    if (lightIndex < static_cast<int>(lights.size())) {
        // Update the light
        lights[lightIndex].position = position;
        lights[lightIndex].intensity = intensity;
        
        // Clear and re-add the lights (this is a simple approach)
        lightingManager->ClearLights();
        for (const auto& light : lights) {
            lightingManager->AddLight(light);
        }
        
        // Update the renderer with the new light data
        SetLightsFromManager(lightingManager);
        
        // Force update the uniform buffer with new light data
        if (m_uniformMapped) {
            UniformBufferObject ubo{};
            ubo.model = m_currentMVP; // Use the stored current MVP matrix as model matrix
            
            // Copy the updated light data
            for (int i = 0; i < 3; ++i) {
                if (i < static_cast<int>(m_lightPositions.size())) {
                    ubo.lightPositions[i] = m_lightPositions[i];
                    ubo.lightColors[i] = m_lightColors[i];
                } else {
                    ubo.lightPositions[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                    ubo.lightColors[i] = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
                }
            }
            
            memcpy(m_uniformMapped, &ubo, sizeof(ubo));
        }
    }
}

void VulkanRenderer::SetLightsFromManager(LightingManager* lightingManager) {
    if (!lightingManager) {
        return;
    }
    
    const auto& lights = lightingManager->GetLights();
    std::vector<glm::vec4> lightPositions;
    std::vector<glm::vec4> lightColors;
    
    for (const auto& light : lights) {
        lightPositions.push_back(glm::vec4(light.position, 1.0f));
        lightColors.push_back(glm::vec4(light.color * light.intensity, 1.0f));
    }
    
    SetLights(lightPositions, lightColors);
}

// ImGui implementation
void VulkanRenderer::InitImGui(GLFWwindow* window) {
    m_window = window;
    
    // Initialize ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
    // Enable resizing from edges for better UX
    io.ConfigWindowsResizeFromEdges = true;
    // Disable multi-viewport to avoid platform window update issues
    // io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    
    // Setup ImGui style with modern dark theme
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 8.0f;
    style.FrameRounding = 4.0f;
    style.GrabRounding = 4.0f;
    style.ScrollbarRounding = 4.0f;
    style.TabRounding = 4.0f;
    
    // Modern color scheme
    style.Colors[ImGuiCol_WindowBg] = ImVec4(0.08f, 0.08f, 0.12f, 0.95f);
    style.Colors[ImGuiCol_Header] = ImVec4(0.15f, 0.15f, 0.20f, 1.0f);
    style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.20f, 0.20f, 0.25f, 1.0f);
    style.Colors[ImGuiCol_Button] = ImVec4(0.20f, 0.40f, 0.80f, 1.0f);
    style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.50f, 0.90f, 1.0f);
    style.Colors[ImGuiCol_Text] = ImVec4(0.90f, 0.90f, 0.90f, 1.0f);
    style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.50f, 0.50f, 0.50f, 1.0f);
    
    // Add visible window borders to show resize affordances
    style.WindowBorderSize = 1.0f;
    
    // Setup Platform/Renderer backends
    ImGui_ImplGlfw_InitForVulkan(window, true);
    
    // Create descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT, 1000 }
    };
    
    VkDescriptorPoolCreateInfo pool_info = {};
    pool_info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    pool_info.maxSets = 1000;
    pool_info.poolSizeCount = std::size(pool_sizes);
    pool_info.pPoolSizes = pool_sizes;
    
    VK_CHECK(vkCreateDescriptorPool(m_dev, &pool_info, nullptr, &m_imguiDescriptorPool));
    
    ImGui_ImplVulkan_InitInfo init_info = {};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_phys;
    init_info.Device = m_dev;
    init_info.QueueFamily = m_queueFamily;
    init_info.Queue = m_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_imguiDescriptorPool;
    init_info.RenderPass = m_renderPass;  // Add the render pass
    init_info.Subpass = 0;
    init_info.MinImageCount = std::max(2u, static_cast<uint32_t>(m_swapchainImages.size()));
    init_info.ImageCount = static_cast<uint32_t>(m_swapchainImages.size());
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;  // match your render pass
    init_info.UseDynamicRendering = VK_FALSE;       // you're using a render pass/framebuffers
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;
    
    // Load Vulkan functions for ImGui BEFORE initializing
    ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void* vulkan_instance) {
        return vkGetInstanceProcAddr(*(VkInstance*)vulkan_instance, function_name);
    }, &m_instance);
    
    // Initialize ImGui Vulkan backend
    ImGui_ImplVulkan_Init(&init_info);
    
    // Upload fonts
    VkCommandBuffer cmd = BeginSingleTimeCommands();
    ImGui_ImplVulkan_CreateFontsTexture();
    EndSingleTimeCommands(cmd);
    
    m_imguiReady = true;
    m_lastFrameTime = glfwGetTime();
    NOVA_INFO("VK: ImGui initialized successfully with modern UI");
    
    // Log sizes before re-sync
    LogSwapchainSizes("Before ImGui re-sync");
    
    // Re-sync once more after ImGui init in case the backend adjusted anything
    SyncPerImageVectors(static_cast<uint32_t>(m_swapchainImages.size()));
    
    // Log sizes after re-sync
    LogSwapchainSizes("After ImGui re-sync");
    
    // Sanity check swapchain sizes after ImGui init
    SanitySwapchainSizes();
}

void VulkanRenderer::UpdatePerformanceMetrics(double deltaTime) {
    m_frameTime = deltaTime * 1000.0; // Convert to milliseconds
    m_frameCount++;
    
    // Update FPS every second
    double currentTime = glfwGetTime();
    if (currentTime - m_fpsUpdateTime >= 1.0) {
        m_fps = m_frameCount / (currentTime - m_fpsUpdateTime);
        m_frameCount = 0;
        m_fpsUpdateTime = currentTime;
    }
}

void VulkanRenderer::BeginFrame() {
    if (m_imguiReady && m_window) {
        ImGui_ImplVulkan_NewFrame();
        // Note: ImGui_ImplGlfw_NewFrame() is called in Editor before this
        ImGui::NewFrame();
    }
}

void VulkanRenderer::EndFrame(VkCommandBuffer cmd) {
    if (m_imguiReady) {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), cmd);
    }
}

void VulkanRenderer::EndFrame() {
    if (m_imguiReady) {
        ImGui::Render();
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), m_cmdBuffer);
    }
}

VkCommandBuffer VulkanRenderer::GetActiveCmd() const {
    return m_cmdBuffer;
}

bool VulkanRenderer::IsImGuiReady() const {
    return m_imguiReady;
}

VkCommandBuffer VulkanRenderer::BeginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = m_cmdPool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(m_dev, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void VulkanRenderer::EndSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(m_queue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(m_queue);

    vkFreeCommandBuffers(m_dev, m_cmdPool, 1, &commandBuffer);
}

void VulkanRenderer::Shutdown() {
    NOVA_INFO("VulkanRenderer::Shutdown: Starting shutdown process");
    
    // Destroy debug messenger
    DestroyDebugMessenger();
    
    if (m_dev != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(m_dev);
    }
    NOVA_INFO("VulkanRenderer::Shutdown: Device idle, shutting down shadow system");
    
    if (m_imguiReady) {
        ImGui_ImplVulkan_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext();
        vkDestroyDescriptorPool(m_dev, m_imguiDescriptorPool, nullptr);
    }
    
    if (m_uniformMapped && m_dev != VK_NULL_HANDLE) {
        vkUnmapMemory(m_dev, m_uniformMemory);
        m_uniformMapped = nullptr;
    }
    
    if (m_uniformBuffer != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_dev, m_uniformBuffer, nullptr);
        m_uniformBuffer = VK_NULL_HANDLE;
    }
    if (m_uniformMemory != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkFreeMemory(m_dev, m_uniformMemory, nullptr);
        m_uniformMemory = VK_NULL_HANDLE;
    }
    
    if (m_indexBuffer != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_dev, m_indexBuffer, nullptr);
        m_indexBuffer = VK_NULL_HANDLE;
    }
    if (m_indexMemory != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkFreeMemory(m_dev, m_indexMemory, nullptr);
        m_indexMemory = VK_NULL_HANDLE;
    }
    
    if (m_vertexBuffer != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkDestroyBuffer(m_dev, m_vertexBuffer, nullptr);
        m_vertexBuffer = VK_NULL_HANDLE;
    }
    if (m_vertexMemory != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkFreeMemory(m_dev, m_vertexMemory, nullptr);
        m_vertexMemory = VK_NULL_HANDLE;
    }
    
    if (m_pipeline != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_dev, m_pipeline, nullptr);
        m_pipeline = VK_NULL_HANDLE;
    }
    if (m_pipelineLayout != VK_NULL_HANDLE && m_dev != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_dev, m_pipelineLayout, nullptr);
        m_pipelineLayout = VK_NULL_HANDLE;
    }
    
    // Cleanup shadow system
    // m_shadowSystem.Shutdown(); // Temporarily disabled
    NOVA_INFO("VulkanRenderer::Shutdown: Shadow system shut down");
    
    if (m_dev != VK_NULL_HANDLE) {
        for (auto framebuffer : m_framebuffers) {
            if (framebuffer != VK_NULL_HANDLE) {
                vkDestroyFramebuffer(m_dev, framebuffer, nullptr);
            }
        }
        m_framebuffers.clear();
        
        if (m_renderPass != VK_NULL_HANDLE) {
            vkDestroyRenderPass(m_dev, m_renderPass, nullptr);
            m_renderPass = VK_NULL_HANDLE;
        }
        
        for (auto imageView : m_swapchainImageViews) {
            if (imageView != VK_NULL_HANDLE) {
                vkDestroyImageView(m_dev, imageView, nullptr);
            }
        }
        m_swapchainImageViews.clear();
        
        if (m_swapchain != VK_NULL_HANDLE) {
            vkDestroySwapchainKHR(m_dev, m_swapchain, nullptr);
            m_swapchain = VK_NULL_HANDLE;
        }
        // Clean up frame-in-flight synchronization objects
        for (uint32_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (m_imageAvailableSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_dev, m_imageAvailableSemaphores[i], nullptr);
                m_imageAvailableSemaphores[i] = VK_NULL_HANDLE;
            }
            if (m_renderFinishedSemaphores[i] != VK_NULL_HANDLE) {
                vkDestroySemaphore(m_dev, m_renderFinishedSemaphores[i], nullptr);
                m_renderFinishedSemaphores[i] = VK_NULL_HANDLE;
            }
            if (m_inFlightFences[i] != VK_NULL_HANDLE) {
                vkDestroyFence(m_dev, m_inFlightFences[i], nullptr);
                m_inFlightFences[i] = VK_NULL_HANDLE;
            }
        }
        m_imageAvailableSemaphores.clear();
        m_renderFinishedSemaphores.clear();
        m_inFlightFences.clear();
        m_commandBuffers.clear();
        m_imagesInFlight.clear();
        if (m_cmdPool != VK_NULL_HANDLE) {
            vkDestroyCommandPool(m_dev, m_cmdPool, nullptr);
            m_cmdPool = VK_NULL_HANDLE;
        }
        
        vkDestroyDevice(m_dev, nullptr);
        m_dev = VK_NULL_HANDLE;
    }
    
    if (m_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(m_instance, nullptr);
        m_instance = VK_NULL_HANDLE;
    }
}

// Shadow mapping implementation
void VulkanRenderer::CreateShadowResources() {
    // This function is now deprecated - shadow resources are managed by ShadowSystem
    NOVA_INFO("CreateShadowResources() is deprecated - using ShadowSystem instead");
}

void VulkanRenderer::CreateShadowPipeline() {
    // This function is now deprecated - shadow pipeline is managed by ShadowSystem
    NOVA_INFO("CreateShadowPipeline() is deprecated - using ShadowSystem instead");
}

void VulkanRenderer::CreateShadowDescriptorSet() {
    // This function is now deprecated - shadow descriptor set is managed by ShadowSystem
    NOVA_INFO("CreateShadowDescriptorSet() is deprecated - using ShadowSystem instead");
}

glm::mat4 VulkanRenderer::CalculateLightSpaceMatrix(const glm::vec3& lightPos) {
    // This function is now deprecated - light space matrix calculation is managed by ShadowSystem
    return glm::mat4(1.0f);
}

glm::mat4 VulkanRenderer::CalculateLightSpaceMatrixForFace(const glm::vec3& lightPos, int face) {
    // This function is now deprecated - light space matrix calculation is managed by ShadowSystem
    return glm::mat4(1.0f);
}

void VulkanRenderer::RenderShadowMaps(VkCommandBuffer cmd, int lightIndex) {
    // This function is now deprecated - shadow rendering is managed by ShadowSystem
    // TODO: Implement proper shadow rendering with the new system
}

} // namespace nova

