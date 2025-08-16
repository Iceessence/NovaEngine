#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include <filesystem>
#include <vector>
#include <functional>

namespace nova {

using AssetGUID = std::string; // Using string for simplicity, could be UUID later

// Forward declarations
class Asset;
class Texture;
class Material;
class Mesh;

enum class AssetType {
    Texture,
    Material,
    Mesh,
    Shader,
    Audio,
    Scene
};

// Base asset class
class Asset {
public:
    AssetGUID guid;
    std::string path;
    AssetType type;
    bool loaded = false;
    std::string lastModified;
    
    virtual ~Asset() = default;
    virtual bool load() = 0;
    virtual void unload() = 0;
};

// Asset registry for tracking dependencies
struct AssetDependency {
    AssetGUID asset;
    std::string path;
    AssetType type;
};

// Asset database entry
struct AssetDBEntry {
    AssetGUID guid;
    std::string path;
    AssetType type;
    std::string lastModified;
    std::vector<AssetDependency> dependencies;
};

class AssetManager {
private:
    std::unordered_map<AssetGUID, std::shared_ptr<Asset>> assets;
    std::unordered_map<std::string, AssetGUID> pathToGUID;
    std::unordered_map<AssetGUID, std::vector<AssetGUID>> dependencyGraph;
    
    std::string assetDBPath = "Assets/assetdb.json";
    std::filesystem::path assetsRoot = "Assets";
    
    // File watcher callback
    std::function<void(const std::string&)> onAssetChanged;

public:
    AssetManager();
    ~AssetManager();
    
    // Core asset management
    AssetGUID registerAsset(const std::string& path, AssetType type);
    std::shared_ptr<Asset> getAsset(const AssetGUID& guid);
    std::shared_ptr<Asset> getAssetByPath(const std::string& path);
    
    // Asset loading/unloading
    bool loadAsset(const AssetGUID& guid);
    void unloadAsset(const AssetGUID& guid);
    void loadAllAssets();
    
    // Dependency tracking
    void addDependency(const AssetGUID& asset, const AssetGUID& dependency);
    std::vector<AssetGUID> getDependencies(const AssetGUID& asset);
    void reloadDependentAssets(const AssetGUID& changedAsset);
    
    // Database persistence
    bool saveAssetDB();
    bool loadAssetDB();
    void scanAssetsDirectory();
    
    // Hot-reload support
    void setAssetChangedCallback(std::function<void(const std::string&)> callback);
    void checkForChanges();
    
    // Utility
    AssetGUID generateGUID(const std::string& path);
    std::string getAssetPath(const AssetGUID& guid);
    std::string normalizePath(const std::string& path);
    bool assetExists(const AssetGUID& guid);
    
    // Builtin assets
    void registerBuiltinAssets();
    
    // Asset type specific getters
    std::shared_ptr<Texture> getTexture(const AssetGUID& guid);
    std::shared_ptr<Material> getMaterial(const AssetGUID& guid);
    std::shared_ptr<Mesh> getMesh(const AssetGUID& guid);
};

} // namespace nova


