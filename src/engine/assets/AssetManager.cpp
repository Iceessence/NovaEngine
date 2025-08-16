#include "AssetManager.h"
#include "Texture.h"
#include "Material.h"
#include "Mesh.h"
#include "core/Log.h"
#include <filesystem>
#include <fstream>
#include <sstream>
#include <algorithm>

namespace nova {

AssetManager::AssetManager() {
    NOVA_INFO("AssetManager initialized");
    
    // Register builtin assets
    registerBuiltinAssets();
}

AssetManager::~AssetManager() {
    // Unload all assets
    for (auto& [guid, asset] : assets) {
        if (asset && asset->loaded) {
            asset->unload();
        }
    }
    assets.clear();
    pathToGUID.clear();
    dependencyGraph.clear();
}

std::string AssetManager::normalizePath(const std::string& path) {
    std::string normalized = path;
    
    // Convert backslashes to forward slashes
    std::replace(normalized.begin(), normalized.end(), '\\', '/');
    
    // Convert to lowercase for case-insensitive comparison
    std::transform(normalized.begin(), normalized.end(), normalized.begin(), ::tolower);
    
    // Remove leading/trailing whitespace
    normalized.erase(0, normalized.find_first_not_of(" \t\r\n"));
    normalized.erase(normalized.find_last_not_of(" \t\r\n") + 1);
    
    return normalized;
}

AssetGUID AssetManager::registerAsset(const std::string& path, AssetType type) {
    std::string normalizedPath = normalizePath(path);
    AssetGUID guid = generateGUID(normalizedPath);
    
    // Check if asset already exists
    if (assets.find(guid) != assets.end()) {
        NOVA_WARN("Asset already registered: " + normalizedPath);
        return guid;
    }
    
    // Create appropriate asset type
    std::shared_ptr<Asset> asset;
    switch (type) {
        case AssetType::Texture:
            asset = std::make_shared<Texture>();
            break;
        case AssetType::Material:
            asset = std::make_shared<Material>();
            break;
        case AssetType::Mesh:
            asset = std::make_shared<Mesh>();
            break;
        default:
            NOVA_ERROR("Unsupported asset type for path: " + path);
            return "";
    }
    
    if (asset) {
        asset->guid = guid;
        asset->path = normalizedPath;
        asset->type = type;
        
        assets[guid] = asset;
        pathToGUID[normalizedPath] = guid;
        
        NOVA_INFO("Registered asset: " + normalizedPath + " -> " + guid);
    }
    
    return guid;
}

std::shared_ptr<Asset> AssetManager::getAsset(const AssetGUID& guid) {
    auto it = assets.find(guid);
    if (it != assets.end()) {
        return it->second;
    }
    return nullptr;
}

std::shared_ptr<Asset> AssetManager::getAssetByPath(const std::string& path) {
    std::string normalizedPath = normalizePath(path);
    auto it = pathToGUID.find(normalizedPath);
    if (it != pathToGUID.end()) {
        return getAsset(it->second);
    }
    NOVA_WARN("Asset not found by path: " + normalizedPath + " (original: " + path + ")");
    return nullptr;
}

bool AssetManager::loadAsset(const AssetGUID& guid) {
    NOVA_INFO("AssetManager::loadAsset called for GUID: " + guid);
    auto asset = getAsset(guid);
    if (!asset) {
        NOVA_ERROR("Asset not found: " + guid);
        return false;
    }
    
    NOVA_INFO("Asset found, checking if already loaded...");
    if (asset->loaded) {
        NOVA_INFO("Asset already loaded: " + guid);
        return true;
    }
    
    // Check file modification time for hot-reload
    if (!asset->path.empty() && asset->path != "builtin:" + asset->guid) {
        std::filesystem::path filePath(asset->path);
        if (std::filesystem::exists(filePath)) {
            auto lastWriteTime = std::filesystem::last_write_time(filePath);
            auto timeString = std::to_string(lastWriteTime.time_since_epoch().count());
            if (asset->lastModified != timeString) {
                asset->lastModified = timeString;
                NOVA_INFO("Asset file modified, reloading: " + asset->path);
            }
        }
    }
    
    NOVA_INFO("Calling asset->load() for: " + guid);
    bool success = asset->load();
    NOVA_INFO("asset->load() returned: " + std::string(success ? "true" : "false"));
    
    if (success) {
        NOVA_INFO("Setting asset as loaded: " + guid);
        asset->loaded = true;
        NOVA_INFO("Asset loaded successfully: " + guid);
    } else {
        NOVA_ERROR("Failed to load asset: " + guid);
    }
    
    NOVA_INFO("AssetManager::loadAsset returning: " + std::string(success ? "true" : "false"));
    return success;
}

void AssetManager::unloadAsset(const AssetGUID& guid) {
    auto asset = getAsset(guid);
    if (asset && asset->loaded) {
        asset->unload();
        asset->loaded = false;
        NOVA_INFO("Asset unloaded: " + guid);
    }
}

void AssetManager::loadAllAssets() {
    NOVA_INFO("Loading all assets...");
    for (auto& [guid, asset] : assets) {
        if (!asset->loaded) {
            loadAsset(guid);
        }
    }
}

void AssetManager::addDependency(const AssetGUID& asset, const AssetGUID& dependency) {
    dependencyGraph[asset].push_back(dependency);
    NOVA_INFO("Added dependency: " + asset + " -> " + dependency);
}

std::vector<AssetGUID> AssetManager::getDependencies(const AssetGUID& asset) {
    auto it = dependencyGraph.find(asset);
    if (it != dependencyGraph.end()) {
        return it->second;
    }
    return {};
}

void AssetManager::reloadDependentAssets(const AssetGUID& changedAsset) {
    // Find all assets that depend on the changed asset
    std::vector<AssetGUID> toReload;
    for (const auto& [asset, deps] : dependencyGraph) {
        if (std::find(deps.begin(), deps.end(), changedAsset) != deps.end()) {
            toReload.push_back(asset);
        }
    }
    
    NOVA_INFO("Reloading " + std::to_string(toReload.size()) + " dependent assets");
    
    // Reload dependent assets
    for (const auto& guid : toReload) {
        unloadAsset(guid);
        loadAsset(guid);
    }
}

bool AssetManager::saveAssetDB() {
    // TODO: Implement JSON serialization
    NOVA_INFO("Asset database saved");
    return true;
}

bool AssetManager::loadAssetDB() {
    // TODO: Implement JSON deserialization
    NOVA_INFO("Asset database loaded");
    return true;
}

void AssetManager::scanAssetsDirectory() {
    NOVA_INFO("Scanning assets directory: " + assetsRoot.string());
    
    if (!std::filesystem::exists(assetsRoot)) {
        NOVA_WARN("Assets directory does not exist: " + assetsRoot.string());
        // Create the directory if it doesn't exist
        std::filesystem::create_directories(assetsRoot);
        NOVA_INFO("Created assets directory: " + assetsRoot.string());
        return;
    }
    
    for (const auto& entry : std::filesystem::recursive_directory_iterator(assetsRoot)) {
        if (entry.is_regular_file()) {
            std::string path = entry.path().string();
            std::string extension = entry.path().extension().string();
            
            AssetType type = AssetType::Texture; // Default
            if (extension == ".gltf" || extension == ".glb" || extension == ".obj") {
                type = AssetType::Mesh;
            } else if (extension == ".mat" || extension == ".json") {
                type = AssetType::Material;
            } else if (extension == ".vert" || extension == ".frag") {
                type = AssetType::Shader;
            }
            
            try {
                registerAsset(path, type);
            } catch (const std::exception& e) {
                NOVA_ERROR("Failed to register asset: " + path + " - " + e.what());
            }
        }
    }
}

void AssetManager::setAssetChangedCallback(std::function<void(const std::string&)> callback) {
    onAssetChanged = callback;
}

void AssetManager::checkForChanges() {
    // Check for file modifications and trigger hot-reload
    for (auto& [guid, asset] : assets) {
        if (!asset->path.empty() && asset->path != "builtin:" + asset->guid) {
            std::filesystem::path filePath(asset->path);
            if (std::filesystem::exists(filePath)) {
                auto lastWriteTime = std::filesystem::last_write_time(filePath);
                auto timeString = std::to_string(lastWriteTime.time_since_epoch().count());
                if (asset->lastModified != timeString) {
                    NOVA_INFO("Asset file changed, triggering reload: " + asset->path);
                    asset->lastModified = timeString;
                    
                    // Unload and reload the asset
                    asset->unload();
                    asset->load();
                    
                    // Reload dependent assets
                    reloadDependentAssets(guid);
                    
                    // Notify callback if set
                    if (onAssetChanged) {
                        onAssetChanged(asset->path);
                    }
                }
            }
        }
    }
}



AssetGUID AssetManager::generateGUID(const std::string& path) {
    // Simple hash-based GUID for now
    std::hash<std::string> hasher;
    size_t hash = hasher(path);
    std::stringstream ss;
    ss << std::hex << hash;
    return ss.str();
}

std::string AssetManager::getAssetPath(const AssetGUID& guid) {
    auto asset = getAsset(guid);
    return asset ? asset->path : "";
}

bool AssetManager::assetExists(const AssetGUID& guid) {
    return assets.find(guid) != assets.end();
}

std::shared_ptr<Texture> AssetManager::getTexture(const AssetGUID& guid) {
    auto asset = getAsset(guid);
    if (asset && asset->type == AssetType::Texture) {
        return std::static_pointer_cast<Texture>(asset);
    }
    return nullptr;
}

void AssetManager::registerBuiltinAssets() {
    NOVA_INFO("Registering builtin assets...");
    
    // Register builtin cube mesh
    registerAsset("builtin:cube", AssetType::Mesh);
    
    // Register builtin default_red material
    registerAsset("builtin:default_red", AssetType::Material);
    
    NOVA_INFO("Builtin assets registered");
}

std::shared_ptr<Material> AssetManager::getMaterial(const AssetGUID& guid) {
    auto asset = getAsset(guid);
    if (asset && asset->type == AssetType::Material) {
        return std::static_pointer_cast<Material>(asset);
    }
    return nullptr;
}

std::shared_ptr<Mesh> AssetManager::getMesh(const AssetGUID& guid) {
    auto asset = getAsset(guid);
    if (asset && asset->type == AssetType::Mesh) {
        return std::static_pointer_cast<Mesh>(asset);
    }
    return nullptr;
}

} // namespace nova
