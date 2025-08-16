#include "HotReload.h"
#include "core/Log.h"

namespace nova {

FileWatcher::FileWatcher(const std::string& path, const std::vector<std::string>& extensions)
    : watchPath(path), fileExtensions(extensions) {
    NOVA_INFO("FileWatcher initialized for path: " + path);
}

FileWatcher::~FileWatcher() {
    stop();
}

void FileWatcher::start() {
    if (running) {
        NOVA_WARN("FileWatcher already running");
        return;
    }
    
    running = true;
    watchThread = std::thread(&FileWatcher::watchLoop, this);
    NOVA_INFO("FileWatcher started");
}

void FileWatcher::stop() {
    if (!running) {
        return;
    }
    
    running = false;
    if (watchThread.joinable()) {
        watchThread.join();
    }
    
    cleanupPlatformWatcher();
    NOVA_INFO("FileWatcher stopped");
}

void FileWatcher::setCallback(std::function<void(const FileChangeEvent&)> cb) {
    std::lock_guard<std::mutex> lock(callbackMutex);
    callback = cb;
}

void FileWatcher::watchLoop() {
    if (!initializePlatformWatcher()) {
        NOVA_ERROR("Failed to initialize platform file watcher");
        return;
    }
    
    // TODO: Implement actual file watching loop
    // For now, just log that we're watching
    NOVA_INFO("FileWatcher loop started");
    
    while (running) {
        // TODO: Check for file changes
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

bool FileWatcher::initializePlatformWatcher() {
    // TODO: Implement platform-specific file watcher initialization
    NOVA_INFO("Platform file watcher initialized (placeholder)");
    return true;
}

void FileWatcher::cleanupPlatformWatcher() {
    // TODO: Implement platform-specific cleanup
    platformHandle = nullptr;
}

HotReloadManager::HotReloadManager(std::shared_ptr<AssetManager> assetMgr)
    : assetManager(assetMgr) {
    NOVA_INFO("HotReloadManager initialized");
}

HotReloadManager::~HotReloadManager() {
    stop();
}

void HotReloadManager::start() {
    if (fileWatcher) {
        NOVA_WARN("HotReloadManager already started");
        return;
    }
    
    fileWatcher = std::make_unique<FileWatcher>("Assets", watchedExtensions);
    fileWatcher->setCallback([this](const FileChangeEvent& event) {
        onFileChanged(event);
    });
    
    fileWatcher->start();
    NOVA_INFO("HotReloadManager started");
}

void HotReloadManager::stop() {
    if (fileWatcher) {
        fileWatcher->stop();
        fileWatcher.reset();
        NOVA_INFO("HotReloadManager stopped");
    }
}

bool HotReloadManager::isRunning() const {
    return fileWatcher && fileWatcher->isRunning();
}

void HotReloadManager::trackAsset(const AssetGUID& guid, const std::string& filePath) {
    fileToAsset[filePath] = guid;
    assetToFile[guid] = filePath;
    NOVA_INFO("Tracking asset: " + filePath + " -> " + guid);
}

void HotReloadManager::untrackAsset(const AssetGUID& guid) {
    auto it = assetToFile.find(guid);
    if (it != assetToFile.end()) {
        fileToAsset.erase(it->second);
        assetToFile.erase(it);
        NOVA_INFO("Untracked asset: " + guid);
    }
}

AssetGUID HotReloadManager::getAssetForFile(const std::string& filePath) const {
    auto it = fileToAsset.find(filePath);
    return it != fileToAsset.end() ? it->second : "";
}

std::string HotReloadManager::getFileForAsset(const AssetGUID& guid) const {
    auto it = assetToFile.find(guid);
    return it != assetToFile.end() ? it->second : "";
}

void HotReloadManager::addWatchedExtension(const std::string& ext) {
    if (std::find(watchedExtensions.begin(), watchedExtensions.end(), ext) == watchedExtensions.end()) {
        watchedExtensions.push_back(ext);
        NOVA_INFO("Added watched extension: " + ext);
    }
}

void HotReloadManager::removeWatchedExtension(const std::string& ext) {
    auto it = std::find(watchedExtensions.begin(), watchedExtensions.end(), ext);
    if (it != watchedExtensions.end()) {
        watchedExtensions.erase(it);
        NOVA_INFO("Removed watched extension: " + ext);
    }
}

bool HotReloadManager::reloadAsset(const AssetGUID& guid) {
    if (!assetManager) {
        NOVA_ERROR("No asset manager available");
        return false;
    }
    
    NOVA_INFO("Reloading asset: " + guid);
    assetManager->unloadAsset(guid);
    return assetManager->loadAsset(guid);
}

bool HotReloadManager::reloadAssetByFile(const std::string& filePath) {
    AssetGUID guid = getAssetForFile(filePath);
    if (guid.empty()) {
        NOVA_ERROR("No asset found for file: " + filePath);
        return false;
    }
    
    return reloadAsset(guid);
}

void HotReloadManager::reloadAllAssets() {
    if (!assetManager) {
        NOVA_ERROR("No asset manager available");
        return;
    }
    
    NOVA_INFO("Reloading all assets");
    assetManager->loadAllAssets();
}

void HotReloadManager::processPendingChanges() {
    std::lock_guard<std::mutex> lock(reloadMutex);
    
    for (const auto& event : pendingChanges) {
        handleAssetReload(event);
    }
    
    pendingChanges.clear();
}

void HotReloadManager::onFileChanged(const FileChangeEvent& event) {
    std::lock_guard<std::mutex> lock(reloadMutex);
    pendingChanges.push_back(event);
    NOVA_INFO("File change detected: " + event.filePath + " (" + std::to_string(static_cast<int>(event.type)) + ")");
}

bool HotReloadManager::handleAssetReload(const FileChangeEvent& event) {
    if (event.type == FileChangeEvent::Type::Modified) {
        if (isShaderFile(event.filePath)) {
            return handleShaderReload(event);
        } else if (isScriptFile(event.filePath)) {
            return handleScriptReload(event);
        } else if (isAssetFile(event.filePath)) {
            return reloadAssetByFile(event.filePath);
        }
    }
    
    return false;
}

bool HotReloadManager::handleShaderReload(const FileChangeEvent& event) {
    NOVA_INFO("Reloading shader: " + event.filePath);
    // TODO: Implement shader hot-reload
    return true;
}

bool HotReloadManager::handleScriptReload(const FileChangeEvent& event) {
    NOVA_INFO("Reloading script: " + event.filePath);
    // TODO: Implement script hot-reload
    return true;
}

bool HotReloadManager::isAssetFile(const std::string& filePath) const {
    std::string ext = filePath.substr(filePath.find_last_of('.'));
    return std::find(watchedExtensions.begin(), watchedExtensions.end(), ext) != watchedExtensions.end();
}

bool HotReloadManager::isShaderFile(const std::string& filePath) const {
    std::string ext = filePath.substr(filePath.find_last_of('.'));
    return ext == ".vert" || ext == ".frag" || ext == ".comp" || ext == ".geom" || ext == ".tesc" || ext == ".tese";
}

bool HotReloadManager::isScriptFile(const std::string& filePath) const {
    std::string ext = filePath.substr(filePath.find_last_of('.'));
    return ext == ".lua" || ext == ".py";
}

AssetType HotReloadManager::getAssetTypeFromFile(const std::string& filePath) const {
    std::string ext = filePath.substr(filePath.find_last_of('.'));
    
    if (ext == ".gltf" || ext == ".glb" || ext == ".obj") {
        return AssetType::Mesh;
    } else if (ext == ".mat" || ext == ".json") {
        return AssetType::Material;
    } else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr") {
        return AssetType::Texture;
    } else if (ext == ".vert" || ext == ".frag") {
        return AssetType::Shader;
    }
    
    return AssetType::Texture; // Default
}

} // namespace nova
