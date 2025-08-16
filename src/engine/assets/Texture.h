#pragma once
#include "AssetManager.h"
#include <glm/glm.hpp>
#include <vector>

namespace nova {

enum class TextureFormat {
    RGBA8,
    RGB8,
    RG8,
    R8,
    RGBA16F,
    RGB16F,
    RG16F,
    R16F,
    RGBA32F,
    RGB32F,
    RG32F,
    R32F,
    D32F,
    D24S8
};

enum class TextureFilter {
    Nearest,
    Linear,
    NearestMipmapNearest,
    LinearMipmapNearest,
    NearestMipmapLinear,
    LinearMipmapLinear
};

enum class TextureWrap {
    ClampToEdge,
    ClampToBorder,
    Repeat,
    MirroredRepeat
};

struct TextureDesc {
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t depth = 1;
    uint32_t mipLevels = 1;
    uint32_t arrayLayers = 1;
    TextureFormat format = TextureFormat::RGBA8;
    TextureFilter minFilter = TextureFilter::Linear;
    TextureFilter magFilter = TextureFilter::Linear;
    TextureWrap wrapU = TextureWrap::Repeat;
    TextureWrap wrapV = TextureWrap::Repeat;
    TextureWrap wrapW = TextureWrap::Repeat;
    bool generateMipmaps = true;
    bool sRGB = false;
};

class Texture : public Asset {
private:
    TextureDesc desc;
    std::vector<uint8_t> data;
    uint32_t vulkanImage = 0; // VkImage handle
    uint32_t vulkanImageView = 0; // VkImageView handle
    uint32_t vulkanSampler = 0; // VkSampler handle
    
public:
    Texture();
    ~Texture();
    
    // Asset interface
    bool load() override;
    void unload() override;
    
    // Texture creation
    bool createFromFile(const std::string& path);
    bool createFromMemory(const void* data, size_t size, const TextureDesc& desc);
    bool createFromData(const std::vector<uint8_t>& pixelData, const TextureDesc& desc);
    
    // Vulkan resource management
    bool createVulkanResources();
    void destroyVulkanResources();
    
    // Getters
    const TextureDesc& getDesc() const { return desc; }
    const std::vector<uint8_t>& getData() const { return data; }
    uint32_t getVulkanImage() const { return vulkanImage; }
    uint32_t getVulkanImageView() const { return vulkanImageView; }
    uint32_t getVulkanSampler() const { return vulkanSampler; }
    
    // Utility
    bool isLoaded() const { return loaded && vulkanImage != 0; }
    uint32_t getWidth() const { return desc.width; }
    uint32_t getHeight() const { return desc.height; }
    uint32_t getMipLevels() const { return desc.mipLevels; }
    
    // Mipmap generation
    bool generateMipmaps();
    
    // Format conversion
    static uint32_t getFormatSize(TextureFormat format);
    static bool isDepthFormat(TextureFormat format);
    static bool isStencilFormat(TextureFormat format);
    static bool isFloatFormat(TextureFormat format);
    
    // Default textures
    static std::shared_ptr<Texture> createDefaultWhite();
    static std::shared_ptr<Texture> createDefaultNormal();
    static std::shared_ptr<Texture> createDefaultBlack();
    static std::shared_ptr<Texture> createDefaultCheckerboard();
};

} // namespace nova


