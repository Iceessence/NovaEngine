#pragma once
#include "../AssetManager.h"
#include "../Mesh.h"
#include "../Material.h"
#include "../Texture.h"
#include <string>
#include <vector>
#include <memory>

namespace nova {

struct GLTFImportOptions {
    bool generateNormals = true;
    bool generateTangents = true;
    bool flipUVs = false;
    bool flipNormals = false;
    float scale = 1.0f;
    bool optimizeMeshes = true;
    bool generateLODs = false;
    uint32_t maxLODLevels = 3;
};

struct GLTFImportResult {
    std::vector<std::shared_ptr<Mesh>> meshes;
    std::vector<std::shared_ptr<Material>> materials;
    std::vector<std::shared_ptr<Texture>> textures;
    std::vector<AssetGUID> meshGUIDs;
    std::vector<AssetGUID> materialGUIDs;
    std::vector<AssetGUID> textureGUIDs;
    bool success = false;
    std::string errorMessage;
};

class GLTFImporter {
private:
    GLTFImportOptions options;
    std::shared_ptr<AssetManager> assetManager;
    
public:
    GLTFImporter(std::shared_ptr<AssetManager> assetMgr);
    ~GLTFImporter();
    
    // Import GLTF file
    GLTFImportResult importFromFile(const std::string& filePath);
    GLTFImportResult importFromMemory(const void* data, size_t size, const std::string& basePath);
    
    // Import options
    void setImportOptions(const GLTFImportOptions& opts) { options = opts; }
    const GLTFImportOptions& getImportOptions() const { return options; }
    
    // Asset manager
    void setAssetManager(std::shared_ptr<AssetManager> assetMgr) { assetManager = assetMgr; }
    
    // Utility functions
    static bool isGLTFFile(const std::string& filePath);
    static bool isGLBFile(const std::string& filePath);
    static std::string getFileExtension(const std::string& filePath);
    
private:
    // Internal import functions
    bool importMeshes(const void* gltfData, GLTFImportResult& result);
    bool importMaterials(const void* gltfData, GLTFImportResult& result);
    bool importTextures(const void* gltfData, GLTFImportResult& result);
    bool importAnimations(const void* gltfData, GLTFImportResult& result);
    
    // Helper functions
    bool processMeshData(const void* meshData, std::shared_ptr<Mesh>& mesh);
    bool processMaterialData(const void* materialData, std::shared_ptr<Material>& material);
    bool processTextureData(const void* textureData, std::shared_ptr<Texture>& texture);
    
    // Geometry processing
    void generateNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void generateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void flipUVs(std::vector<Vertex>& vertices);
    void flipNormals(std::vector<Vertex>& vertices);
    void scaleVertices(std::vector<Vertex>& vertices, float scale);
    
    // Validation
    bool validateGLTFData(const void* data, size_t size);
    std::string getLastError() const;
};

} // namespace nova

