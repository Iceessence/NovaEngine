#ifndef VK_NO_PROTOTYPES
#define VK_NO_PROTOTYPES
#endif
#include <volk.h>
#include <cstdio>
#include <algorithm>
#include <cstdio>
namespace nova { bool ShouldDisableImGui(); }
// Patched VulkanRenderer.cpp (ImGui enablement + safe NewFrame/Render path)
// Drop-in replacement for: src/engine/renderer/vk/VulkanRenderer.cpp
// Notes:
// - Treats NOVA_DISABLE_IMGUI empty/"0"/"false" as NOT disabled
// - Calls ImGui backends NewFrame/Render when enabled
// - Does not call deprecated font upload destroy funcs (works with newer ImGui backends)
// - Externs g_ui_frame_begun so Editor can safely guard UI calls

#include "VulkanRenderer.h"
#include <cstdio>
#include "VulkanHelpers.h"
#include <cstdio>
#include "core/Log.h"
#include <cstdio>

#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cstdio>

#include <imgui.h>
#include <cstdio>
#include <backends/imgui_impl_glfw.h>
#include <cstdio>
#include <backends/imgui_impl_vulkan.h>
#include <cstdio>

#include <algorithm>
#include <cstdio>
namespace nova {

// This symbol is defined (or shimmed) in Editor side to gate UI drawing.
extern bool g_ui_frame_begun;

static bool env_truthy(const char* name)
{
    const char* v = std::getenv(name);
    if (!v) return false;
    // Treat "", "0", "false" (any case) as false
    if (v[0] == '\0') return false;
    if ((v[0] == '0' && v[1] == '\0')) return false;
    if (_stricmp(v, "false") == 0) return false;
    return true;
}

void VulkanRenderer::InitImGui(GLFWwindow* window)
{
    const bool disableImGui = env_truthy("NOVA_DISABLE_IMGUI");
    if (disableImGui)
    {
        NOVA_INFO("VK: Skipping ImGui init via NOVA_DISABLE_IMGUI");
        m_imguiReady = false;
        return;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;

    // Platform
    ImGui_ImplGlfw_InitForVulkan(window, true);

    // Descriptor pool for ImGui
    VkDescriptorPoolSize pool_sizes[] = {
        { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLER, 1000 },
        { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, 1000 },
        { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, 1000 },
        { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1000 },
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

    // Renderer
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance = m_instance;
    init_info.PhysicalDevice = m_phys;
    init_info.Device = m_dev;
    init_info.QueueFamily = m_queueFamily;
    init_info.Queue = m_queue;
    init_info.PipelineCache = VK_NULL_HANDLE;
    init_info.DescriptorPool = m_imguiPool;
    init_info.Allocator = nullptr;
    init_info.MinImageCount = (uint32_t)m_images.size();
    init_info.ImageCount = (uint32_t)m_images.size();
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.CheckVkResultFn = [](VkResult res){ VK_CHECK(res); };

    {
    ImGui_ImplVulkan_InitInfo init_info{};
    init_info.Instance       = m_instance;
    init_info.PhysicalDevice = m_phys;
    init_info.Device         = m_dev;
    init_info.QueueFamily    = m_queueFamily;
    init_info.Queue          = m_queue;
    init_info.DescriptorPool = m_imguiPool;
    init_info.MinImageCount  = (uint32_t) (m_images.size() > 0 ? m_images.size() : 2);
    init_info.ImageCount     = init_info.MinImageCount;
    init_info.MSAASamples    = VK_SAMPLE_COUNT_1_BIT;
    init_info.Subpass        = 0;
    init_info.UseDynamicRendering = VK_FALSE;
    // init_info.CheckVkResultFn = [](VkResult r){ if(r) fprintf(stderr, "VK err %d\n", r); };

    if (!ImGui_ImplVulkan_Init(&init_info)) { std::fprintf(stderr,"ImGui_ImplVulkan_Init failed\n"); std::abort(); }
}// Newer ImGui backends expose zero-arg CreateFontsTexture
    // (older had VkCommandBuffer arg). We call the zero-arg one.
    (void)ImGui_ImplVulkan_CreateFontsTexture();

    m_imguiReady = true;
    NOVA_INFO("VK: ImGui initialized");
}

void VulkanRenderer::BeginFrame()
{
    // ... existing acquire/sync work before recording command buffer ...

    if (m_imguiReady)
    {
        ImGui_ImplVulkan_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        g_ui_frame_begun = true;
    }
}

void VulkanRenderer::EndFrame(VkCommandBuffer cmd)
{
    if (m_imguiReady)
    {
        ImGui::Render();
        ImDrawData* dd = ImGui::GetDrawData();
        ImGui_ImplVulkan_RenderDrawData(dd, cmd);
        g_ui_frame_begun = false;
    }

    // ... existing submit/present work ...
}

} // namespace nova

namespace nova {

bool ShouldDisableImGui()
{
    const char* e = std::getenv("NOVA_DISABLE_IMGUI");
    if (!e || !*e) return false;
    std::string v(e);
    std::transform(v.begin(), v.end(), v.begin(), ::tolower);
    auto ltrim = [](std::string& s){ s.erase(0, s.find_first_not_of(" \t\r\n")); };
    auto rtrim = [](std::string& s){ s.erase(s.find_last_not_of(" \t\r\n") + 1); };
    ltrim(v); rtrim(v);
    return (v=="1" || v=="true" || v=="yes" || v=="on");
}

} // namespace nova

#ifndef VK_CHECK
#define VK_CHECK(x) do { VkResult _err = (x); if (_err != VK_SUCCESS) { /* TODO: replace with your logger */ __debugbreak(); } } while(0)
#endif


/* removed duplicate IsImGuiReady body (handled inline in header) */

bool nova::VulkanRenderer::IsImGuiReady() const { return m_imguiReady; }



