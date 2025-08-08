#pragma once
#ifndef VK_CHECK
#define VK_CHECK(expr) do { VkResult _vk__res = (expr); if (_vk__res != VK_SUCCESS) { /* log if you have a macro */ /*fprintf(stderr,"VK error %d at %s:%d\n", (int)_vk__res, __FILE__, __LINE__);*/ __debugbreak(); } } while(0)
#endif
#include <volk.h>
#include <vector>
#include <string>
namespace nova::vkutil {
struct ShaderModule { VkShaderModule module{}; };
ShaderModule LoadShader(VkDevice device, const std::string& path);
}
