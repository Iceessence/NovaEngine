#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <volk.h>

#include "VulkanRenderer.h"
#include "imgui.h"
#include "imgui_impl_vulkan.h"
#include "core/Log.h"
#include "VulkanHelpers.h"

#include <imgui.h>
#include <backends/imgui_impl_glfw.h>
#include <backends/imgui_impl_vulkan.h>

#include <cstdlib>
#include <algorithm>
#include <cstdio>

namespace nova {

// Optional flag you can read from elsewhere if you want to gate UI drawing.
bool g_ui_frame_begun = false;

static bool EnvTruthy(const char* name)
{
    const char* v = std::getenv(name);
    if (!v) return false;
    if (v[0] == '\0') return false;
    if ((v[0] == '0' && v[1] == '\0')) return false;
    // case-insensitive "false"
    if ((v[0]=='f'||v[0]=='F') && (v[1]=='a'||v[1]=='A') && (v[2]=='l'||v[2]=='L') &&
        (v[3]=='s'||v[3]=='S') && (v[4]=='e'||v[4]=='E') && v[5]=='\0') return false;
    return true;
}

void VulkanRenderer::InitImGui(GLFWwindow* window)
{
    if (EnvTruthy("NOVA_DISABLE_IMGUI")) {
        NOVA_INFO("VK: ImGui disabled by NOVA_DISABLE_IMGUI");
        m_imguiReady = false;
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Platform backend
    ImGui_ImplGlfw_InitForVulkan(window, true);

    // Descriptor pool dedicated for ImGui
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
    VkDescriptorPoolCreateInfo dpci{};
    dpci.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    dpci.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
    dpci.maxSets = 1000 * (uint32_t)(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
    dpci.poolSizeCount = (uint32_t)(sizeof(pool_sizes) / sizeof(pool_sizes[0]));
    dpci.pPoolSizes = pool_sizes;
    VK_CHECK(vkCreateDescriptorPool(m_dev, &dpci, nullptr, &m_imguiPool));

    // Renderer backend (classic render-pass path; your header doesn't expose ColorAttachmentFormat)
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance       = m_instance;
    init_info.PhysicalDevice = m_phys;
    init_info.Device         = m_dev;
    init_info.QueueFamily    = m_queueFamily;
    init_info.Queue          = m_queue;
    init_info.PipelineCache  = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_imguiPool;
    init_info.RenderPass     = m_renderPass;          // <- classic path
    init_info.Subpass        = 0;
    init_info.MinImageCount  = (uint32_t)(m_images.size() ? m_images.size() : 2);
    init_info.ImageCount     = init_info.MinImageCount;
    init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator      = nullptr;
    init_info.CheckVkResultFn = [](VkResult r){ VK_CHECK(r); };

    if (!ImGui_ImplVulkan_Init(&init_info)) {
        std::fprintf(stderr, "ImGui_ImplVulkan_Init failed\n");
        std::abort();
    }

    // Upload fonts (zero-arg overload present in your backend)
    ImGui_ImplVulkan_CreateFontsTexture();

    m_imguiReady = true;
    NOVA_INFO("VK: ImGui initialized (render-pass path)");
}

void VulkanRenderer::BeginFrame()
{
    // ... your existing acquire/sync code above command recording ...

    if (m_imguiReady) {
        ImGui_ImplGlfw_NewFrame();
        ImGui_ImplVulkan_NewFrame();
        ImGui::NewFrame();              // <-- ADD THIS
        g_ui_frame_begun = true;
    }
}

void VulkanRenderer::EndFrame(VkCommandBuffer cmd)
{
    // ... your normal scene draws recorded to 'cmd' are already here ...

    if (m_imguiReady && ImGui::GetCurrentContext()) {
        ImGui::Render();                                        // <-- ADD THIS
        ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(),   // keep this
            cmd);
    }

    // IMPORTANT: this block must be INSIDE the render pass / dynamic rendering.
    // So it should be placed just before your:
    //   vkCmdEndRenderPass(cmd);
    // or:
    //   vkCmdEndRendering(cmd / vkCmdEndRenderingKHR)
    // and then your submit/present happens after.
}


bool VulkanRenderer::IsImGuiReady() const { return m_imguiReady; }

} // namespace nova
