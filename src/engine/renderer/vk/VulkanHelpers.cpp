#include "VulkanHelpers.h"
#include <fstream>
#include <stdexcept>
namespace nova::vkutil {
ShaderModule LoadShader(VkDevice device, const std::string& path){
    std::ifstream f(path, std::ios::binary);
    if(!f) throw std::runtime_error("Failed to open shader: " + path);
    std::vector<char> bytes((std::istreambuf_iterator<char>(f)), {});
    VkShaderModuleCreateInfo ci{ VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
    ci.codeSize = bytes.size();
    ci.pCode = reinterpret_cast<const uint32_t*>(bytes.data());
    ShaderModule out{};
    if(vkCreateShaderModule(device, &ci, nullptr, &out.module)!=VK_SUCCESS)
        throw std::runtime_error("vkCreateShaderModule failed");
    return out;
}
}
