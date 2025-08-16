#include "GLTFImporter.h"
#include "core/Log.h"
// #include "cgltf.h"
#include <fstream>
#include <sstream>

namespace nova {

GLTFImporter::GLTFImporter(std::shared_ptr<AssetManager> assetMgr) 
    : assetManager(assetMgr) {
    NOVA_INFO("GLTFImporter initialized");
}

GLTFImporter::~GLTFImporter() {
}

GLTFImportResult GLTFImporter::importFromFile(const std::string& filePath) {
    NOVA_INFO("Importing GLTF file: " + filePath);
    
    GLTFImportResult result;
    
    // Check if file exists
    if (!std::filesystem::exists(filePath)) {
        result.errorMessage = "File does not exist: " + filePath;
        NOVA_ERROR("GLTF import failed: " + result.errorMessage);
        return result;
    }
    
    // For now, create a simple sphere mesh programmatically
    // TODO: Implement proper GLTF parsing when cgltf is working
    NOVA_INFO("Creating sphere mesh programmatically for: " + filePath);
    
    auto sphereMesh = std::make_shared<Mesh>();
    
    // Create sphere geometry
    const float PI = 3.14159265359f;
    const int segments = 16;
    const int rings = 16;
    const float radius = 1.0f;
    
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    
    // Generate vertices
    for (int ring = 0; ring <= rings; ++ring) {
        float phi = ring * PI / rings;
        float sinPhi = sin(phi);
        float cosPhi = cos(phi);
        
        for (int segment = 0; segment <= segments; ++segment) {
            float theta = segment * 2.0f * PI / segments;
            float sinTheta = sin(theta);
            float cosTheta = cos(theta);
            
            Vertex vertex;
            vertex.position = glm::vec3(
                radius * sinPhi * cosTheta,
                radius * cosPhi,
                radius * sinPhi * sinTheta
            );
            vertex.normal = glm::normalize(vertex.position);
            vertex.uv0 = glm::vec2(
                static_cast<float>(segment) / segments,
                static_cast<float>(ring) / rings
            );
            vertex.uv1 = vertex.uv0;
            vertex.tangent = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
            vertex.color0 = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            vertex.color1 = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            vertex.joints0 = glm::uvec4(0, 0, 0, 0);
            vertex.weights0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            
            vertices.push_back(vertex);
        }
    }
    
    // Generate indices
    for (int ring = 0; ring < rings; ++ring) {
        for (int segment = 0; segment < segments; ++segment) {
            int current = ring * (segments + 1) + segment;
            int next = current + segments + 1;
            
            indices.push_back(current);
            indices.push_back(next);
            indices.push_back(current + 1);
            
            indices.push_back(next);
            indices.push_back(next + 1);
            indices.push_back(current + 1);
        }
    }
    
    // Set mesh data
    sphereMesh->createFromVertices(vertices, indices);
    
    // Generate GUID and register with asset manager
    std::string meshName = "sphere_from_gltf";
    AssetGUID meshGUID = assetManager->generateGUID(meshName);
    sphereMesh->guid = meshGUID;
    sphereMesh->path = meshName;
    sphereMesh->type = AssetType::Mesh;
    
    result.meshes.push_back(sphereMesh);
    result.meshGUIDs.push_back(meshGUID);
    result.success = true;
    
    NOVA_INFO("Created sphere mesh: " + meshName + " (" + std::to_string(vertices.size()) + " vertices, " + std::to_string(indices.size()) + " indices)");
    
    return result;
}

GLTFImportResult GLTFImporter::importFromMemory(const void* data, size_t size, const std::string& basePath) {
    GLTFImportResult result;
    
    NOVA_INFO("Importing GLTF from memory: " + std::to_string(size) + " bytes");
    
    // TODO: Implement actual GLTF loading from memory
    result.success = false;
    result.errorMessage = "GLTF import from memory not yet implemented";
    
    return result;
}

bool GLTFImporter::isGLTFFile(const std::string& filePath) {
    std::string ext = getFileExtension(filePath);
    return ext == ".gltf";
}

bool GLTFImporter::isGLBFile(const std::string& filePath) {
    std::string ext = getFileExtension(filePath);
    return ext == ".glb";
}

std::string GLTFImporter::getFileExtension(const std::string& filePath) {
    size_t pos = filePath.find_last_of('.');
    if (pos != std::string::npos) {
        return filePath.substr(pos);
    }
    return "";
}

bool GLTFImporter::importMeshes(const void* gltfData, GLTFImportResult& result) {
    // GLTF mesh import temporarily disabled - using programmatic sphere generation
    NOVA_WARN("GLTF mesh import temporarily disabled");
    return false;
}

bool GLTFImporter::importMaterials(const void* gltfData, GLTFImportResult& result) {
    // GLTF material import temporarily disabled - using programmatic sphere generation
    NOVA_WARN("GLTF material import temporarily disabled");
    return false;
}

bool GLTFImporter::importTextures(const void* gltfData, GLTFImportResult& result) {
    // GLTF texture import temporarily disabled - using programmatic sphere generation
    NOVA_WARN("GLTF texture import temporarily disabled");
    return false;
}

bool GLTFImporter::importAnimations(const void* gltfData, GLTFImportResult& result) {
    // TODO: Implement animation import
    return false;
}

bool GLTFImporter::processMeshData(const void* meshData, std::shared_ptr<Mesh>& mesh) {
    // TODO: Implement mesh data processing
    return false;
}

bool GLTFImporter::processMaterialData(const void* materialData, std::shared_ptr<Material>& material) {
    // TODO: Implement material data processing
    return false;
}

bool GLTFImporter::processTextureData(const void* textureData, std::shared_ptr<Texture>& texture) {
    // TODO: Implement texture data processing
    return false;
}

void GLTFImporter::generateNormals(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    // Reset all normals to zero
    for (auto& vertex : vertices) {
        vertex.normal = glm::vec3(0.0f);
    }
    
    // Calculate normals from triangles
    for (size_t i = 0; i < indices.size(); i += 3) {
        if (i + 2 >= indices.size()) break;
        
        uint32_t i0 = indices[i];
        uint32_t i1 = indices[i + 1];
        uint32_t i2 = indices[i + 2];
        
        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size()) continue;
        
        glm::vec3 v0 = vertices[i0].position;
        glm::vec3 v1 = vertices[i1].position;
        glm::vec3 v2 = vertices[i2].position;
        
        glm::vec3 edge1 = v1 - v0;
        glm::vec3 edge2 = v2 - v0;
        glm::vec3 normal = glm::normalize(glm::cross(edge1, edge2));
        
        vertices[i0].normal += normal;
        vertices[i1].normal += normal;
        vertices[i2].normal += normal;
    }
    
    // Normalize all normals
    for (auto& vertex : vertices) {
        if (glm::length(vertex.normal) > 0.0f) {
            vertex.normal = glm::normalize(vertex.normal);
        } else {
            vertex.normal = glm::vec3(0.0f, 1.0f, 0.0f); // Default up normal
        }
    }
    
    NOVA_INFO("Generated normals for " + std::to_string(vertices.size()) + " vertices");
}

void GLTFImporter::generateTangents(std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    // TODO: Implement tangent generation
}

void GLTFImporter::flipUVs(std::vector<Vertex>& vertices) {
    for (auto& vertex : vertices) {
        vertex.uv0.y = 1.0f - vertex.uv0.y;
        vertex.uv1.y = 1.0f - vertex.uv1.y;
    }
    NOVA_INFO("Flipped UVs for " + std::to_string(vertices.size()) + " vertices");
}

void GLTFImporter::flipNormals(std::vector<Vertex>& vertices) {
    for (auto& vertex : vertices) {
        vertex.normal = -vertex.normal;
    }
    NOVA_INFO("Flipped normals for " + std::to_string(vertices.size()) + " vertices");
}

void GLTFImporter::scaleVertices(std::vector<Vertex>& vertices, float scale) {
    for (auto& vertex : vertices) {
        vertex.position *= scale;
    }
    NOVA_INFO("Scaled vertices by " + std::to_string(scale) + " for " + std::to_string(vertices.size()) + " vertices");
}

bool GLTFImporter::validateGLTFData(const void* data, size_t size) {
    // TODO: Implement GLTF validation
    return false;
}

std::string GLTFImporter::getLastError() const {
    return "GLTF importer not yet implemented";
}

} // namespace nova
