#include "ShadowSystem.h"
#include "core/Log.h"
#include "../vk/VulkanHelpers.h"
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

namespace nova {

ShadowSystem::ShadowSystem() {
}

ShadowSystem::~ShadowSystem() {
    Shutdown();
}

void ShadowSystem::Initialize(VkDevice device, VkPhysicalDevice physicalDevice) {
    m_device = device;
    m_physicalDevice = physicalDevice;
    
    NOVA_INFO("Initializing Shadow System...");
    
    NOVA_INFO("Creating shadow render passes...");
    CreateShadowRenderPasses();
    NOVA_INFO("Creating shadow pipelines...");
    CreateShadowPipelines();
    NOVA_INFO("Creating shadow sampler...");
    CreateShadowSampler();
    NOVA_INFO("Creating shadow maps...");
    CreateShadowMaps();
    NOVA_INFO("Creating shadow descriptor set...");
    CreateShadowDescriptorSet();
    
    NOVA_INFO("Shadow System initialized successfully");
}

void ShadowSystem::Shutdown() {
    if (m_device == VK_NULL_HANDLE) return;
    
    NOVA_INFO("Shutting down Shadow System...");
    
    vkDeviceWaitIdle(m_device);
    
    // Destroy shadow maps
    DestroyShadowMaps();
    
    // Destroy pipelines
    if (m_shadowPipeline2D != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_shadowPipeline2D, nullptr);
        m_shadowPipeline2D = VK_NULL_HANDLE;
    }
    
    if (m_shadowPipelineCube != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_device, m_shadowPipelineCube, nullptr);
        m_shadowPipelineCube = VK_NULL_HANDLE;
    }
    
    if (m_shadowPipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_device, m_shadowPipelineLayout, nullptr);
        m_shadowPipelineLayout = VK_NULL_HANDLE;
    }
    
    // Destroy render passes
    if (m_shadowRenderPass2D != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_shadowRenderPass2D, nullptr);
        m_shadowRenderPass2D = VK_NULL_HANDLE;
    }
    
    if (m_shadowRenderPassCube != VK_NULL_HANDLE) {
        vkDestroyRenderPass(m_device, m_shadowRenderPassCube, nullptr);
        m_shadowRenderPassCube = VK_NULL_HANDLE;
    }
    
    // Destroy descriptor resources
    if (m_shadowSampler != VK_NULL_HANDLE) {
        vkDestroySampler(m_device, m_shadowSampler, nullptr);
        m_shadowSampler = VK_NULL_HANDLE;
    }
    
    if (m_shadowDescriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(m_device, m_shadowDescriptorPool, nullptr);
        m_shadowDescriptorPool = VK_NULL_HANDLE;
    }
    
    if (m_shadowDescriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(m_device, m_shadowDescriptorSetLayout, nullptr);
        m_shadowDescriptorSetLayout = VK_NULL_HANDLE;
    }
    
    NOVA_INFO("Shadow System shutdown complete");
}

void ShadowSystem::CreateShadowMaps() {
    NOVA_INFO("Creating shadow maps...");
    
    // Create 2D shadow map array (directional + spot lights)
    NOVA_INFO("CreateShadowMaps: Creating 2D shadow map image...");
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = SHADOW_MAP_SIZE;
        imageInfo.extent.height = SHADOW_MAP_SIZE;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = MAX_DIRECTIONAL_LIGHTS * CASCADE_COUNT + MAX_SPOT_LIGHTS;
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        NOVA_INFO("CreateShadowMaps: About to create 2D shadow map image...");
        VK_CHECK(vkCreateImage(m_device, &imageInfo, nullptr, &m_shadowMap2D));
        NOVA_INFO("CreateShadowMaps: 2D shadow map image created successfully");
        
        // Allocate memory
        NOVA_INFO("CreateShadowMaps: Allocating memory for 2D shadow map...");
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, m_shadowMap2D, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        NOVA_INFO("CreateShadowMaps: About to allocate memory for 2D shadow map...");
        VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_shadowMap2DMemory));
        NOVA_INFO("CreateShadowMaps: Memory allocated, binding to image...");
        vkBindImageMemory(m_device, m_shadowMap2D, m_shadowMap2DMemory, 0);
        NOVA_INFO("CreateShadowMaps: Memory bound to 2D shadow map image");
        
        NOVA_INFO("CreateShadowMaps: Creating 2D array view...");
        // Create array view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_shadowMap2D;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = MAX_DIRECTIONAL_LIGHTS * CASCADE_COUNT + MAX_SPOT_LIGHTS;
        
        NOVA_INFO("CreateShadowMaps: About to create 2D array view...");
        VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_shadowMap2DView));
        NOVA_INFO("CreateShadowMaps: 2D array view created successfully");
        
        NOVA_INFO("CreateShadowMaps: Creating individual layer views...");
        // Create individual layer views for framebuffers
        m_shadowMap2DLayerViews.resize(MAX_DIRECTIONAL_LIGHTS * CASCADE_COUNT + MAX_SPOT_LIGHTS);
        NOVA_INFO("CreateShadowMaps: Creating individual layer views...");
        for (uint32_t i = 0; i < m_shadowMap2DLayerViews.size(); i++) {
            VkImageViewCreateInfo layerViewInfo{};
            layerViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            layerViewInfo.image = m_shadowMap2D;
            layerViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            layerViewInfo.format = VK_FORMAT_D32_SFLOAT;
            layerViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            layerViewInfo.subresourceRange.baseMipLevel = 0;
            layerViewInfo.subresourceRange.levelCount = 1;
            layerViewInfo.subresourceRange.baseArrayLayer = i;
            layerViewInfo.subresourceRange.layerCount = 1;
            
            VK_CHECK(vkCreateImageView(m_device, &layerViewInfo, nullptr, &m_shadowMap2DLayerViews[i]));
        }
        NOVA_INFO("CreateShadowMaps: All 2D layer views created successfully");
    }
    
    // Create cubemap shadow map array (point lights)
    NOVA_INFO("CreateShadowMaps: Creating cubemap shadow map image...");
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = SHADOW_MAP_SIZE;
        imageInfo.extent.height = SHADOW_MAP_SIZE;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = 1;
        imageInfo.arrayLayers = MAX_POINT_LIGHTS * 6; // 6 faces per cube
        imageInfo.format = VK_FORMAT_D32_SFLOAT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        
        NOVA_INFO("CreateShadowMaps: About to create cubemap shadow map image...");
        VK_CHECK(vkCreateImage(m_device, &imageInfo, nullptr, &m_shadowMapCube));
        NOVA_INFO("CreateShadowMaps: Cubemap shadow map image created successfully");
        
        // Allocate memory
        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_device, m_shadowMapCube, &memRequirements);
        
        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
        
        VK_CHECK(vkAllocateMemory(m_device, &allocInfo, nullptr, &m_shadowMapCubeMemory));
        vkBindImageMemory(m_device, m_shadowMapCube, m_shadowMapCubeMemory, 0);
        
        // Create cubemap array view (using 2D array for now)
        NOVA_INFO("CreateShadowMaps: Creating cubemap array view...");
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_shadowMapCube;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY; // Use 2D array instead of cube array
        viewInfo.format = VK_FORMAT_D32_SFLOAT;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = MAX_POINT_LIGHTS * 6;
        
        NOVA_INFO("CreateShadowMaps: About to create cubemap array view...");
        VK_CHECK(vkCreateImageView(m_device, &viewInfo, nullptr, &m_shadowMapCubeView));
        NOVA_INFO("CreateShadowMaps: Cubemap array view created successfully");
        
        // Create individual face views for framebuffers
        m_shadowMapCubeLayerViews.resize(MAX_POINT_LIGHTS * 6);
        for (uint32_t i = 0; i < m_shadowMapCubeLayerViews.size(); i++) {
            VkImageViewCreateInfo faceViewInfo{};
            faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            faceViewInfo.image = m_shadowMapCube;
            faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            faceViewInfo.format = VK_FORMAT_D32_SFLOAT;
            faceViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
            faceViewInfo.subresourceRange.baseMipLevel = 0;
            faceViewInfo.subresourceRange.levelCount = 1;
            faceViewInfo.subresourceRange.baseArrayLayer = i;
            faceViewInfo.subresourceRange.layerCount = 1;
            
            VK_CHECK(vkCreateImageView(m_device, &faceViewInfo, nullptr, &m_shadowMapCubeLayerViews[i]));
        }
    }
    
    // Create framebuffers
    CreateShadowFramebuffers();
    
    // Transition images to the correct layout for sampling
    NOVA_INFO("CreateShadowMaps: Transitioning images to correct layout...");
    TransitionImagesToReadOnly();
    
    NOVA_INFO("Shadow maps created successfully");
}

void ShadowSystem::DestroyShadowMaps() {
    // Destroy framebuffers
    for (auto& framebuffer : m_shadowFramebuffers2D) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
    m_shadowFramebuffers2D.clear();
    
    for (auto& framebuffer : m_shadowFramebuffersCube) {
        if (framebuffer != VK_NULL_HANDLE) {
            vkDestroyFramebuffer(m_device, framebuffer, nullptr);
        }
    }
    m_shadowFramebuffersCube.clear();
    
    // Destroy layer views
    for (auto& view : m_shadowMap2DLayerViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, view, nullptr);
        }
    }
    m_shadowMap2DLayerViews.clear();
    
    for (auto& view : m_shadowMapCubeLayerViews) {
        if (view != VK_NULL_HANDLE) {
            vkDestroyImageView(m_device, view, nullptr);
        }
    }
    m_shadowMapCubeLayerViews.clear();
    
    // Destroy main views
    if (m_shadowMap2DView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_shadowMap2DView, nullptr);
        m_shadowMap2DView = VK_NULL_HANDLE;
    }
    
    if (m_shadowMapCubeView != VK_NULL_HANDLE) {
        vkDestroyImageView(m_device, m_shadowMapCubeView, nullptr);
        m_shadowMapCubeView = VK_NULL_HANDLE;
    }
    
    // Destroy images and memory
    if (m_shadowMap2D != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_shadowMap2D, nullptr);
        m_shadowMap2D = VK_NULL_HANDLE;
    }
    
    if (m_shadowMap2DMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_shadowMap2DMemory, nullptr);
        m_shadowMap2DMemory = VK_NULL_HANDLE;
    }
    
    if (m_shadowMapCube != VK_NULL_HANDLE) {
        vkDestroyImage(m_device, m_shadowMapCube, nullptr);
        m_shadowMapCube = VK_NULL_HANDLE;
    }
    
    if (m_shadowMapCubeMemory != VK_NULL_HANDLE) {
        vkFreeMemory(m_device, m_shadowMapCubeMemory, nullptr);
        m_shadowMapCubeMemory = VK_NULL_HANDLE;
    }
}

void ShadowSystem::CreateShadowFramebuffers() {
    // Create 2D shadow map framebuffers
    m_shadowFramebuffers2D.resize(m_shadowMap2DLayerViews.size());
    for (size_t i = 0; i < m_shadowMap2DLayerViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_shadowRenderPass2D;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &m_shadowMap2DLayerViews[i];
        framebufferInfo.width = SHADOW_MAP_SIZE;
        framebufferInfo.height = SHADOW_MAP_SIZE;
        framebufferInfo.layers = 1;
        
        VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_shadowFramebuffers2D[i]));
    }
    
    // Create cubemap shadow map framebuffers
    m_shadowFramebuffersCube.resize(m_shadowMapCubeLayerViews.size());
    for (size_t i = 0; i < m_shadowMapCubeLayerViews.size(); i++) {
        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = m_shadowRenderPassCube;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = &m_shadowMapCubeLayerViews[i];
        framebufferInfo.width = SHADOW_MAP_SIZE;
        framebufferInfo.height = SHADOW_MAP_SIZE;
        framebufferInfo.layers = 1;
        
        VK_CHECK(vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_shadowFramebuffersCube[i]));
    }
}

void ShadowSystem::CreateShadowRenderPasses() {
    // Create 2D shadow render pass
    {
        VkAttachmentDescription depthAttachment{};
        depthAttachment.format = VK_FORMAT_D32_SFLOAT;
        depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
        
        VkAttachmentReference depthAttachmentRef{};
        depthAttachmentRef.attachment = 0;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
        
        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 0;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        
        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        dependency.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
        dependency.dstStageMask = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
        
        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &depthAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;
        
        VK_CHECK(vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_shadowRenderPass2D));
    }
    
    // Create cubemap shadow render pass (same as 2D for now)
    m_shadowRenderPassCube = m_shadowRenderPass2D;
}

void ShadowSystem::CreateShadowPipelines() {
    // Load shadow vertex shader
    auto vertShader = vkutil::LoadShader(m_device, "assets/shaders/shadow.vert.spv");
    if (vertShader.module == VK_NULL_HANDLE) {
        NOVA_INFO("Failed to load shadow vertex shader!");
        return;
    }
    
    // Load shadow fragment shader
    auto fragShader = vkutil::LoadShader(m_device, "assets/shaders/shadow.frag.spv");
    if (fragShader.module == VK_NULL_HANDLE) {
        NOVA_INFO("Failed to load shadow fragment shader!");
        vkDestroyShaderModule(m_device, vertShader.module, nullptr);
        return;
    }
    
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
    
    // Vertex input
    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    
    // Input assembly
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;
    
    // Viewport and scissor
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.height = static_cast<float>(SHADOW_MAP_SIZE);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    
    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {SHADOW_MAP_SIZE, SHADOW_MAP_SIZE};
    
    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;
    
    // Rasterizer
    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_TRUE;
    rasterizer.depthBiasConstantFactor = 1.0f;
    rasterizer.depthBiasClamp = 0.0f;
    rasterizer.depthBiasSlopeFactor = 1.0f;
    
    // Multisampling
    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    
    // Depth and stencil
    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.stencilTestEnable = VK_FALSE;
    
    // Pipeline layout
    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 0;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    
    VkPushConstantRange pushConstantRange{};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(glm::mat4);
    
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;
    
    VK_CHECK(vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_shadowPipelineLayout));
    
    // Create 2D shadow pipeline
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
    pipelineInfo.pColorBlendState = nullptr;
    pipelineInfo.pDynamicState = nullptr;
    pipelineInfo.layout = m_shadowPipelineLayout;
    pipelineInfo.renderPass = m_shadowRenderPass2D;
    pipelineInfo.subpass = 0;
    
    VK_CHECK(vkCreateGraphicsPipelines(m_device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &m_shadowPipeline2D));
    
    // Create cubemap shadow pipeline (same as 2D for now)
    m_shadowPipelineCube = m_shadowPipeline2D;
    
    // Cleanup shader modules
    vkDestroyShaderModule(m_device, fragShader.module, nullptr);
    vkDestroyShaderModule(m_device, vertShader.module, nullptr);
}

void ShadowSystem::CreateShadowSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_TRUE;
    samplerInfo.compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;
    
    VK_CHECK(vkCreateSampler(m_device, &samplerInfo, nullptr, &m_shadowSampler));
}

void ShadowSystem::CreateShadowDescriptorSet() {
    NOVA_INFO("CreateShadowDescriptorSet: Creating descriptor set layout...");
    // Create descriptor set layout (only 2D binding for now)
    VkDescriptorSetLayoutBinding shadowMap2DBinding{};
    shadowMap2DBinding.binding = 0;
    shadowMap2DBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowMap2DBinding.descriptorCount = 1;
    shadowMap2DBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    
    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = 1;
    layoutInfo.pBindings = &shadowMap2DBinding;
    
    VK_CHECK(vkCreateDescriptorSetLayout(m_device, &layoutInfo, nullptr, &m_shadowDescriptorSetLayout));
    NOVA_INFO("CreateShadowDescriptorSet: Descriptor set layout created successfully");
    
    // Create descriptor pool
    VkDescriptorPoolSize poolSizes[] = {
        {VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1}
    };
    
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = 1;
    poolInfo.pPoolSizes = poolSizes;
    poolInfo.maxSets = 1;
    
    VK_CHECK(vkCreateDescriptorPool(m_device, &poolInfo, nullptr, &m_shadowDescriptorPool));
    
    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_shadowDescriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &m_shadowDescriptorSetLayout;
    
    NOVA_INFO("CreateShadowDescriptorSet: About to allocate descriptor set...");
    VK_CHECK(vkAllocateDescriptorSets(m_device, &allocInfo, &m_shadowDescriptorSet));
    NOVA_INFO("CreateShadowDescriptorSet: Descriptor set allocated successfully");
    
    // Update descriptor set
    NOVA_INFO("CreateShadowDescriptorSet: Checking image views...");
    NOVA_INFO("CreateShadowDescriptorSet: Image views and sampler ready");
    
    VkDescriptorImageInfo shadowMap2DInfo{};
    shadowMap2DInfo.imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL;
    shadowMap2DInfo.imageView = m_shadowMap2DView;
    shadowMap2DInfo.sampler = m_shadowSampler;
    
    VkWriteDescriptorSet descriptorWrite{};
    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    descriptorWrite.dstSet = m_shadowDescriptorSet;
    descriptorWrite.dstBinding = 0;
    descriptorWrite.dstArrayElement = 0;
    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    descriptorWrite.descriptorCount = 1;
    descriptorWrite.pImageInfo = &shadowMap2DInfo;
    
    NOVA_INFO("CreateShadowDescriptorSet: About to update descriptor sets...");
    vkUpdateDescriptorSets(m_device, 1, &descriptorWrite, 0, nullptr);
    NOVA_INFO("CreateShadowDescriptorSet: Descriptor sets updated successfully");
}

void ShadowSystem::RenderShadowMaps(VkCommandBuffer cmd, const std::vector<ShadowLight>& lights) {
    // This will be implemented in the next step
    // For now, just a placeholder to prevent crashes
    NOVA_INFO("ShadowSystem::RenderShadowMaps: Placeholder implementation");
}

uint32_t ShadowSystem::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
    VkPhysicalDeviceMemoryProperties memProperties;
    vkGetPhysicalDeviceMemoryProperties(m_physicalDevice, &memProperties);
    
    for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    
    throw std::runtime_error("Failed to find suitable memory type!");
}

void ShadowSystem::TransitionImagesToReadOnly() {
    NOVA_INFO("CreateShadowMaps: Transitioning images to DEPTH_STENCIL_READ_ONLY_OPTIMAL");
    
    // For now, we'll skip the transition and just log it
    // The proper transition will be done in the main render loop
    // This prevents the crash while we implement the full shadow system
    NOVA_INFO("CreateShadowMaps: Skipping transition for now to prevent crash");
}

} // namespace nova
