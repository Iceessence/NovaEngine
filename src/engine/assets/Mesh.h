#pragma once
#include "AssetManager.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace nova {

enum class VertexAttribute {
    Position,
    Normal,
    Tangent,
    UV0,
    UV1,
    Color0,
    Color1,
    Joints0,
    Weights0
};

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec4 tangent; // w component stores handedness
    glm::vec2 uv0;
    glm::vec2 uv1;
    glm::vec4 color0;
    glm::vec4 color1;
    glm::uvec4 joints0;
    glm::vec4 weights0;
};

struct Submesh {
    std::string name;
    uint32_t indexOffset = 0;
    uint32_t indexCount = 0;
    uint32_t vertexOffset = 0;
    uint32_t vertexCount = 0;
    AssetGUID materialGUID;
    glm::mat4 transform = glm::mat4(1.0f);
    bool visible = true;
};

struct BoundingBox {
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(std::numeric_limits<float>::lowest());
    
    void expand(const glm::vec3& point);
    void expand(const BoundingBox& other);
    glm::vec3 getCenter() const;
    glm::vec3 getExtent() const;
    float getRadius() const;
};

struct BoundingSphere {
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.0f;
    
    void expand(const glm::vec3& point);
    void expand(const BoundingSphere& other);
};

class Mesh : public Asset {
private:
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
    std::vector<Submesh> submeshes;
    
    BoundingBox boundingBox;
    BoundingSphere boundingSphere;
    
    // Vulkan resources
    uint32_t vertexBuffer = 0; // VkBuffer handle
    uint32_t indexBuffer = 0;  // VkBuffer handle
    uint32_t vertexBufferMemory = 0;
    uint32_t indexBufferMemory = 0;
    
    // GPU instancing support
    uint32_t instanceBuffer = 0;
    uint32_t instanceBufferMemory = 0;
    std::vector<glm::mat4> instanceTransforms;
    
public:
    Mesh();
    ~Mesh();
    
    // Asset interface
    bool load() override;
    void unload() override;
    
    // Mesh creation and loading
    bool loadFromFile(const std::string& path);
    bool saveToFile(const std::string& path);
    
    // Geometry creation
    bool createFromVertices(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds);
    bool createFromPositions(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& indices);
    
    // Vulkan resource management
    bool createVulkanResources();
    void destroyVulkanResources();
    bool updateInstanceBuffer(const std::vector<glm::mat4>& transforms);
    
    // Asset system integration
    std::vector<float> getVertexDataForRenderer() const;
    std::vector<uint32_t> getIndexDataForRenderer() const;
    
    // Geometry access
    const std::vector<Vertex>& getVertices() const { return vertices; }
    const std::vector<uint32_t>& getIndices() const { return indices; }
    const std::vector<Submesh>& getSubmeshes() const { return submeshes; }
    
    // Submesh management
    void addSubmesh(const Submesh& submesh);
    Submesh& getSubmesh(uint32_t index);
    const Submesh& getSubmesh(uint32_t index) const;
    uint32_t getSubmeshCount() const { return submeshes.size(); }
    
    // Bounding volume access
    const BoundingBox& getBoundingBox() const { return boundingBox; }
    const BoundingSphere& getBoundingSphere() const { return boundingSphere; }
    
    // Vulkan resource access
    uint32_t getVertexBuffer() const { return vertexBuffer; }
    uint32_t getIndexBuffer() const { return indexBuffer; }
    uint32_t getInstanceBuffer() const { return instanceBuffer; }
    
    // Utility
    uint32_t getVertexCount() const { return vertices.size(); }
    uint32_t getIndexCount() const { return indices.size(); }
    bool hasIndices() const { return !indices.empty(); }
    bool isLoaded() const { return loaded && vertexBuffer != 0; }
    
    // Bounding volume computation
    void computeBoundingVolumes();
    
    // Instance geometry creation
    void createCube(float size = 1.0f);
    
    // Default meshes
    static std::shared_ptr<Mesh> createCubeStatic(float size = 1.0f);
    static std::shared_ptr<Mesh> createSphere(float radius = 0.5f, uint32_t segments = 32);
    static std::shared_ptr<Mesh> createPlane(float width = 1.0f, float height = 1.0f);
    static std::shared_ptr<Mesh> createCylinder(float radius = 0.5f, float height = 1.0f, uint32_t segments = 32);
    static std::shared_ptr<Mesh> createCone(float radius = 0.5f, float height = 1.0f, uint32_t segments = 32);
    static std::shared_ptr<Mesh> createTorus(float outerRadius = 0.5f, float innerRadius = 0.2f, uint32_t segments = 32, uint32_t sides = 16);
    
    // Validation
    bool isValid() const;
    std::vector<std::string> getValidationErrors() const;
};

} // namespace nova
