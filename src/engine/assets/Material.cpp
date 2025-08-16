#include "Material.h"
#include "core/Log.h"

namespace nova {

Material::Material() {
    type = AssetType::Material;
}

Material::~Material() {
    unload();
}

bool Material::load() {
    if (path.empty()) {
        NOVA_ERROR("Material path is empty");
        return false;
    }
    
    NOVA_INFO("Loading material: " + path);
    
    // Handle builtin materials
    if (path.find("builtin:") == 0) {
        if (path == "builtin:default_red") {
            // Create a red material
            params.baseColor = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            params.metallic = 0.0f;
            params.roughness = 0.5f;
            params.normalScale = 1.0f;
            params.occlusionStrength = 1.0f;
            params.emissive = glm::vec3(0.0f);
            params.emissiveIntensity = 1.0f;
            
            // Clear texture references for builtin materials
            baseColorTexture = "";
            metallicRoughnessTexture = "";
            normalTexture = "";
            occlusionTexture = "";
            emissiveTexture = "";
            
            loaded = true;
            NOVA_INFO("Loaded builtin red material");
            return true;
        }
    }
    
    // TODO: Implement actual material loading from JSON/TOML
    NOVA_INFO("Loading material: " + path);
    // For now, create a default PBR material
    params.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    params.metallic = 0.0f;
    params.roughness = 0.5f;
    params.normalScale = 1.0f;
    params.occlusionStrength = 1.0f;
    params.emissive = glm::vec3(0.0f);
    params.emissiveIntensity = 1.0f;
    
    loaded = true;
    return true;
}

void Material::unload() {
    // Clear texture references
    baseColorTex.reset();
    metallicRoughnessTex.reset();
    normalTex.reset();
    occlusionTex.reset();
    emissiveTex.reset();
    clearcoatTex.reset();
    clearcoatRoughnessTex.reset();
    transmissionTex.reset();
    thicknessTex.reset();
    
    loaded = false;
}

bool Material::loadFromFile(const std::string& filePath) {
    path = filePath;
    return load();
}

bool Material::saveToFile(const std::string& filePath) {
    // TODO: Implement material serialization to JSON/TOML
    NOVA_INFO("Saving material to: " + filePath);
    return true;
}

void Material::setBaseColorTexture(const AssetGUID& textureGUID) {
    baseColorTexture = textureGUID;
    // TODO: Add dependency tracking when AssetManager is available
    // if (!textureGUID.empty()) {
    //     // assetManager->addDependency(guid, textureGUID);
    // }
}

void Material::setMetallicRoughnessTexture(const AssetGUID& textureGUID) {
    metallicRoughnessTexture = textureGUID;
}

void Material::setNormalTexture(const AssetGUID& textureGUID) {
    normalTexture = textureGUID;
}

void Material::setOcclusionTexture(const AssetGUID& textureGUID) {
    occlusionTexture = textureGUID;
}

void Material::setEmissiveTexture(const AssetGUID& textureGUID) {
    emissiveTexture = textureGUID;
}

void Material::setClearcoatTexture(const AssetGUID& textureGUID) {
    clearcoatTexture = textureGUID;
}

void Material::setClearcoatRoughnessTexture(const AssetGUID& textureGUID) {
    clearcoatRoughnessTexture = textureGUID;
}

void Material::setTransmissionTexture(const AssetGUID& textureGUID) {
    transmissionTexture = textureGUID;
}

void Material::setThicknessTexture(const AssetGUID& textureGUID) {
    thicknessTexture = textureGUID;
}

std::shared_ptr<Material> Material::createDefaultPBR() {
    auto material = std::make_shared<Material>();
    material->shadingModel = MaterialShadingModel::PBR;
    material->params.baseColor = glm::vec4(1.0f, 0.8f, 0.6f, 1.0f); // Warm white
    material->params.metallic = 0.0f;
    material->params.roughness = 0.5f;
    material->params.normalScale = 1.0f;
    material->params.occlusionStrength = 1.0f;
    material->params.emissive = glm::vec3(0.0f);
    material->params.emissiveIntensity = 1.0f;
    material->params.blendMode = MaterialBlendMode::Opaque;
    
    // Create default textures (these will be created when the material is loaded)
    // For now, we'll use empty GUIDs to prevent crashes
    material->baseColorTexture = "";
    material->metallicRoughnessTexture = "";
    material->normalTexture = "";
    material->occlusionTexture = "";
    material->emissiveTexture = "";
    
    material->loaded = true;
    return material;
}

std::shared_ptr<Material> Material::createDefaultUnlit() {
    auto material = std::make_shared<Material>();
    material->shadingModel = MaterialShadingModel::Unlit;
    material->params.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    material->params.blendMode = MaterialBlendMode::Opaque;
    material->loaded = true;
    return material;
}

std::shared_ptr<Material> Material::createDefaultLit() {
    auto material = std::make_shared<Material>();
    material->shadingModel = MaterialShadingModel::Lit;
    material->params.baseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    material->params.roughness = 0.5f;
    material->params.blendMode = MaterialBlendMode::Opaque;
    material->loaded = true;
    return material;
}

bool Material::isValid() const {
    // Basic validation
    if (params.roughness < 0.0f || params.roughness > 1.0f) {
        return false;
    }
    if (params.metallic < 0.0f || params.metallic > 1.0f) {
        return false;
    }
    if (params.alphaCutoff < 0.0f || params.alphaCutoff > 1.0f) {
        return false;
    }
    return true;
}

std::vector<std::string> Material::getValidationErrors() const {
    std::vector<std::string> errors;
    
    if (params.roughness < 0.0f || params.roughness > 1.0f) {
        errors.push_back("Roughness must be between 0.0 and 1.0");
    }
    if (params.metallic < 0.0f || params.metallic > 1.0f) {
        errors.push_back("Metallic must be between 0.0 and 1.0");
    }
    if (params.alphaCutoff < 0.0f || params.alphaCutoff > 1.0f) {
        errors.push_back("Alpha cutoff must be between 0.0 and 1.0");
    }
    
    return errors;
}

} // namespace nova
