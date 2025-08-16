#pragma once
#include "AssetManager.h"
#include "Texture.h"
#include <glm/glm.hpp>
#include <memory>

namespace nova {

enum class MaterialBlendMode {
    Opaque,
    Masked,
    Translucent,
    Additive
};

enum class MaterialShadingModel {
    Unlit,
    Lit,
    PBR
};

struct MaterialParams {
    // Base material properties
    glm::vec4 baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    float metallic = 0.0f;
    float roughness = 0.5f;
    float normalScale = 1.0f;
    float occlusionStrength = 1.0f;
    glm::vec3 emissive = glm::vec3(0.0f);
    float emissiveIntensity = 1.0f;
    
    // PBR specific
    float clearcoat = 0.0f;
    float clearcoatRoughness = 0.0f;
    float anisotropy = 0.0f;
    glm::vec3 anisotropyDirection = glm::vec3(1.0f, 0.0f, 0.0f);
    float ior = 1.5f;
    float transmission = 0.0f;
    float thickness = 0.0f;
    float attenuationDistance = 0.0f;
    glm::vec3 attenuationColor = glm::vec3(1.0f);
    
    // Alpha and blending
    float alphaCutoff = 0.5f;
    MaterialBlendMode blendMode = MaterialBlendMode::Opaque;
    bool doubleSided = false;
    
    // UV transforms
    glm::vec2 uvOffset = glm::vec2(0.0f);
    glm::vec2 uvScale = glm::vec2(1.0f);
    float uvRotation = 0.0f;
};

class Material : public Asset {
private:
    MaterialParams params;
    MaterialShadingModel shadingModel = MaterialShadingModel::PBR;
    
    // Texture references (GUIDs)
    AssetGUID baseColorTexture;
    AssetGUID metallicRoughnessTexture;
    AssetGUID normalTexture;
    AssetGUID occlusionTexture;
    AssetGUID emissiveTexture;
    AssetGUID clearcoatTexture;
    AssetGUID clearcoatRoughnessTexture;
    AssetGUID transmissionTexture;
    AssetGUID thicknessTexture;
    
    // Cached texture pointers
    std::shared_ptr<Texture> baseColorTex;
    std::shared_ptr<Texture> metallicRoughnessTex;
    std::shared_ptr<Texture> normalTex;
    std::shared_ptr<Texture> occlusionTex;
    std::shared_ptr<Texture> emissiveTex;
    std::shared_ptr<Texture> clearcoatTex;
    std::shared_ptr<Texture> clearcoatRoughnessTex;
    std::shared_ptr<Texture> transmissionTex;
    std::shared_ptr<Texture> thicknessTex;
    
public:
    Material();
    ~Material();
    
    // Asset interface
    bool load() override;
    void unload() override;
    
    // Material creation and loading
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    
    // Parameter access
    MaterialParams& getParams() { return params; }
    const MaterialParams& getParams() const { return params; }
    
    MaterialShadingModel getShadingModel() const { return shadingModel; }
    void setShadingModel(MaterialShadingModel model) { shadingModel = model; }
    
    // Texture management
    void setBaseColorTexture(const AssetGUID& textureGUID);
    void setMetallicRoughnessTexture(const AssetGUID& textureGUID);
    void setNormalTexture(const AssetGUID& textureGUID);
    void setOcclusionTexture(const AssetGUID& textureGUID);
    void setEmissiveTexture(const AssetGUID& textureGUID);
    void setClearcoatTexture(const AssetGUID& textureGUID);
    void setClearcoatRoughnessTexture(const AssetGUID& textureGUID);
    void setTransmissionTexture(const AssetGUID& textureGUID);
    void setThicknessTexture(const AssetGUID& textureGUID);
    
    // Texture getters
    AssetGUID getBaseColorTexture() const { return baseColorTexture; }
    AssetGUID getMetallicRoughnessTexture() const { return metallicRoughnessTexture; }
    AssetGUID getNormalTexture() const { return normalTexture; }
    AssetGUID getOcclusionTexture() const { return occlusionTexture; }
    AssetGUID getEmissiveTexture() const { return emissiveTexture; }
    AssetGUID getClearcoatTexture() const { return clearcoatTexture; }
    AssetGUID getClearcoatRoughnessTexture() const { return clearcoatRoughnessTexture; }
    AssetGUID getTransmissionTexture() const { return transmissionTexture; }
    AssetGUID getThicknessTexture() const { return thicknessTexture; }
    
    // Cached texture access
    std::shared_ptr<Texture> getBaseColorTex() const { return baseColorTex; }
    std::shared_ptr<Texture> getMetallicRoughnessTex() const { return metallicRoughnessTex; }
    std::shared_ptr<Texture> getNormalTex() const { return normalTex; }
    std::shared_ptr<Texture> getOcclusionTex() const { return occlusionTex; }
    std::shared_ptr<Texture> getEmissiveTex() const { return emissiveTex; }
    std::shared_ptr<Texture> getClearcoatTex() const { return clearcoatTex; }
    std::shared_ptr<Texture> getClearcoatRoughnessTex() const { return clearcoatRoughnessTex; }
    std::shared_ptr<Texture> getTransmissionTex() const { return transmissionTex; }
    std::shared_ptr<Texture> getThicknessTex() const { return thicknessTex; }
    
    // Utility
    bool isLoaded() const { return loaded; }
    bool hasBaseColorTexture() const { return !baseColorTexture.empty(); }
    bool hasMetallicRoughnessTexture() const { return !metallicRoughnessTexture.empty(); }
    bool hasNormalTexture() const { return !normalTexture.empty(); }
    bool hasOcclusionTexture() const { return !occlusionTexture.empty(); }
    bool hasEmissiveTexture() const { return !emissiveTexture.empty(); }
    
    // Default materials
    static std::shared_ptr<Material> createDefaultPBR();
    static std::shared_ptr<Material> createDefaultUnlit();
    static std::shared_ptr<Material> createDefaultLit();
    
    // Validation
    bool isValid() const;
    std::vector<std::string> getValidationErrors() const;
};

} // namespace nova
