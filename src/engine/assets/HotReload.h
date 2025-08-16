#pragma once
#include "AssetManager.h"
#include <string>
#include <vector>
#include <unordered_map>
#include <functional>
#include <thread>
#include <atomic>
#include <mutex>

namespace nova {

struct FileChangeEvent {
    std::string filePath;
    std::string oldPath; // for rename events
    enum class Type {
        Created,
        Modified,
        Deleted,
        Renamed
    } type;
    uint64_t timestamp;
};

class FileWatcher {
private:
    std::string watchPath;
    std::vector<std::string> fileExtensions;
    std::function<void(const FileChangeEvent&)> callback;
    
    std::thread watchThread;
    std::atomic<bool> running{false};
    std::mutex callbackMutex;
    
    // Platform-specific handles
    void* platformHandle = nullptr;
    
public:
    FileWatcher(const std::string& path, const std::vector<std::string>& extensions = {});
    ~FileWatcher();
    
    void start();
    void stop();
    bool isRunning() const { return running; }
    
    void setCallback(std::function<void(const FileChangeEvent&)> cb);
    
private:
    void watchLoop();
    bool initializePlatformWatcher();
    void cleanupPlatformWatcher();
};

class HotReloadManager {
private:
    std::shared_ptr<AssetManager> assetManager;
    std::unique_ptr<FileWatcher> fileWatcher;
    
    std::unordered_map<std::string, AssetGUID> fileToAsset;
    std::unordered_map<AssetGUID, std::string> assetToFile;
    
    std::vector<std::string> watchedExtensions = {
        ".gltf", ".glb", ".fbx", ".obj",
        ".png", ".jpg", ".jpeg", ".tga", ".bmp", ".hdr",
        ".mat", ".json", ".toml",
        ".vert", ".frag", ".comp", ".geom", ".tesc", ".tese",
        ".lua", ".py"
    };
    
    std::mutex reloadMutex;
    std::vector<FileChangeEvent> pendingChanges;
    
public:
    HotReloadManager(std::shared_ptr<AssetManager> assetMgr);
    ~HotReloadManager();
    
    // Hot-reload management
    void start();
    void stop();
    bool isRunning() const;
    
    // Asset tracking
    void trackAsset(const AssetGUID& guid, const std::string& filePath);
    void untrackAsset(const AssetGUID& guid);
    AssetGUID getAssetForFile(const std::string& filePath) const;
    std::string getFileForAsset(const AssetGUID& guid) const;
    
    // File watching configuration
    void addWatchedExtension(const std::string& ext);
    void removeWatchedExtension(const std::string& ext);
    const std::vector<std::string>& getWatchedExtensions() const { return watchedExtensions; }
    
    // Manual reload
    bool reloadAsset(const AssetGUID& guid);
    bool reloadAssetByFile(const std::string& filePath);
    void reloadAllAssets();
    
    // Processing
    void processPendingChanges();
    
private:
    void onFileChanged(const FileChangeEvent& event);
    bool handleAssetReload(const FileChangeEvent& event);
    bool handleShaderReload(const FileChangeEvent& event);
    bool handleScriptReload(const FileChangeEvent& event);
    
    // Utility
    bool isAssetFile(const std::string& filePath) const;
    bool isShaderFile(const std::string& filePath) const;
    bool isScriptFile(const std::string& filePath) const;
    AssetType getAssetTypeFromFile(const std::string& filePath) const;
};

} // namespace nova


