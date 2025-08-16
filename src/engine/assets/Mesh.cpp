#include "Mesh.h"
#include "engine/core/Log.h"
#include <algorithm>
#include <limits>

namespace nova {

// BoundingBox implementation
void BoundingBox::expand(const glm::vec3& point) {
    min = glm::min(min, point);
    max = glm::max(max, point);
}

void BoundingBox::expand(const BoundingBox& other) {
    min = glm::min(min, other.min);
    max = glm::max(max, other.max);
}

glm::vec3 BoundingBox::getCenter() const {
    return (min + max) * 0.5f;
}

glm::vec3 BoundingBox::getExtent() const {
    return (max - min) * 0.5f;
}

float BoundingBox::getRadius() const {
    return glm::length(getExtent());
}

// BoundingSphere implementation
void BoundingSphere::expand(const glm::vec3& point) {
    glm::vec3 diff = point - center;
    float distSq = glm::dot(diff, diff);
    if (distSq > radius * radius) {
        float dist = sqrt(distSq);
        float newRadius = (radius + dist) * 0.5f;
        center += diff * ((newRadius - radius) / dist);
        radius = newRadius;
    }
}

void BoundingSphere::expand(const BoundingSphere& other) {
    glm::vec3 diff = other.center - center;
    float distSq = glm::dot(diff, diff);
    if (distSq > (radius + other.radius) * (radius + other.radius)) {
        float dist = sqrt(distSq);
        float newRadius = (radius + other.radius + dist) * 0.5f;
        center += diff * ((newRadius - radius) / dist);
        radius = newRadius;
    }
}

// Mesh implementation
Mesh::Mesh() {
    type = AssetType::Mesh;
    NOVA_INFO("Mesh created");
}

Mesh::~Mesh() {
    destroyVulkanResources();
}

bool Mesh::load() {
    if (path.empty()) {
        NOVA_ERROR("Mesh path is empty");
        return false;
    }
    
    NOVA_INFO("Loading mesh: " + path);
    
    // Handle builtin meshes
    if (path.find("builtin:") == 0) {
        if (path == "builtin:cube") {
            // Create a simple cube mesh
            createCube();
            loaded = true;
            NOVA_INFO("Loaded builtin cube mesh");
            return true;
        }
    }
    
    // Basic implementation - just mark as loaded
    loaded = true;
    return true;
}

void Mesh::unload() {
    destroyVulkanResources();
    vertices.clear();
    indices.clear();
    submeshes.clear();
    loaded = false;
}

bool Mesh::loadFromFile(const std::string& path) {
    // Basic implementation - would normally load from file
    NOVA_INFO("Loading mesh from: " + path);
    return load();
}

bool Mesh::saveToFile(const std::string& path) {
    // Basic implementation - would normally save to file
    NOVA_INFO("Saving mesh to: " + path);
    return true;
}

bool Mesh::createFromVertices(const std::vector<Vertex>& verts, const std::vector<uint32_t>& inds) {
    vertices = verts;
    indices = inds;
    computeBoundingVolumes();
    return true;
}

bool Mesh::createFromPositions(const std::vector<glm::vec3>& positions, const std::vector<uint32_t>& inds) {
    vertices.clear();
    vertices.reserve(positions.size());
    
    for (const auto& pos : positions) {
        Vertex v;
        v.position = pos;
        v.normal = glm::vec3(0, 1, 0); // Default normal
        v.tangent = glm::vec4(1, 0, 0, 1); // Default tangent
        v.uv0 = glm::vec2(0, 0); // Default UV
        v.uv1 = glm::vec2(0, 0);
        v.color0 = glm::vec4(1, 1, 1, 1); // Default color
        v.color1 = glm::vec4(0, 0, 0, 0);
        v.joints0 = glm::uvec4(0, 0, 0, 0);
        v.weights0 = glm::vec4(1, 0, 0, 0);
        vertices.push_back(v);
    }
    
    indices = inds;
    computeBoundingVolumes();
    return true;
}

bool Mesh::createVulkanResources() {
    // Basic implementation - would normally create Vulkan buffers
    NOVA_INFO("Creating Vulkan resources for mesh");
    return true;
}

void Mesh::destroyVulkanResources() {
    // Basic implementation - would normally destroy Vulkan buffers
    vertexBuffer = 0;
    indexBuffer = 0;
    vertexBufferMemory = 0;
    indexBufferMemory = 0;
    instanceBuffer = 0;
    instanceBufferMemory = 0;
}

bool Mesh::updateInstanceBuffer(const std::vector<glm::mat4>& transforms) {
    instanceTransforms = transforms;
    return true;
}

std::vector<float> Mesh::getVertexDataForRenderer() const {
    std::vector<float> data;
    data.reserve(vertices.size() * 8); // position(3) + normal(3) + uv(2)
    
    for (const auto& vertex : vertices) {
        // Position
        data.push_back(vertex.position.x);
        data.push_back(vertex.position.y);
        data.push_back(vertex.position.z);
        
        // Normal
        data.push_back(vertex.normal.x);
        data.push_back(vertex.normal.y);
        data.push_back(vertex.normal.z);
        
        // UV
        data.push_back(vertex.uv0.x);
        data.push_back(vertex.uv0.y);
    }
    
    return data;
}

std::vector<uint32_t> Mesh::getIndexDataForRenderer() const {
    return indices;
}

void Mesh::addSubmesh(const Submesh& submesh) {
    submeshes.push_back(submesh);
}

Submesh& Mesh::getSubmesh(uint32_t index) {
    if (index >= submeshes.size()) {
        static Submesh empty;
        return empty;
    }
    return submeshes[index];
}

const Submesh& Mesh::getSubmesh(uint32_t index) const {
    if (index >= submeshes.size()) {
        static const Submesh empty;
        return empty;
    }
    return submeshes[index];
}

void Mesh::computeBoundingVolumes() {
    if (vertices.empty()) {
        boundingBox = BoundingBox();
        boundingSphere = BoundingSphere();
        return;
    }
    
    // Compute bounding box
    boundingBox = BoundingBox();
    for (const auto& vertex : vertices) {
        boundingBox.expand(vertex.position);
    }
    
    // Compute bounding sphere
    boundingSphere.center = boundingBox.getCenter();
    boundingSphere.radius = 0.0f;
    for (const auto& vertex : vertices) {
        boundingSphere.expand(vertex.position);
    }
}

void Mesh::createCube(float size) {
    std::vector<glm::vec3> positions = {
        // Front face
        {-size, -size,  size}, { size, -size,  size}, { size,  size,  size}, {-size,  size,  size},
        // Back face
        {-size, -size, -size}, {-size,  size, -size}, { size,  size, -size}, { size, -size, -size},
        // Top face
        {-size,  size, -size}, {-size,  size,  size}, { size,  size,  size}, { size,  size, -size},
        // Bottom face
        {-size, -size, -size}, { size, -size, -size}, { size, -size,  size}, {-size, -size,  size},
        // Right face
        { size, -size, -size}, { size,  size, -size}, { size,  size,  size}, { size, -size,  size},
        // Left face
        {-size, -size, -size}, {-size, -size,  size}, {-size,  size,  size}, {-size,  size, -size}
    };
    
    std::vector<uint32_t> indices = {
        0,  1,  2,    0,  2,  3,   // front
        4,  5,  6,    4,  6,  7,   // back
        8,  9,  10,   8,  10, 11,  // top
        12, 13, 14,   12, 14, 15,  // bottom
        16, 17, 18,   16, 18, 19,  // right
        20, 21, 22,   20, 22, 23   // left
    };
    
    createFromPositions(positions, indices);
}

std::shared_ptr<Mesh> Mesh::createCubeStatic(float size) {
    auto mesh = std::make_shared<Mesh>();
    
    std::vector<glm::vec3> positions = {
        // Front face
        {-size, -size,  size}, { size, -size,  size}, { size,  size,  size}, {-size,  size,  size},
        // Back face
        {-size, -size, -size}, {-size,  size, -size}, { size,  size, -size}, { size, -size, -size},
        // Top face
        {-size,  size, -size}, {-size,  size,  size}, { size,  size,  size}, { size,  size, -size},
        // Bottom face
        {-size, -size, -size}, { size, -size, -size}, { size, -size,  size}, {-size, -size,  size},
        // Right face
        { size, -size, -size}, { size,  size, -size}, { size,  size,  size}, { size, -size,  size},
        // Left face
        {-size, -size, -size}, {-size, -size,  size}, {-size,  size,  size}, {-size,  size, -size}
    };
    
    std::vector<uint32_t> indices = {
        0,  1,  2,    0,  2,  3,   // front
        4,  5,  6,    4,  6,  7,   // back
        8,  9,  10,   8,  10, 11,  // top
        12, 13, 14,   12, 14, 15,  // bottom
        16, 17, 18,   16, 18, 19,  // right
        20, 21, 22,   20, 22, 23   // left
    };
    
    mesh->createFromPositions(positions, indices);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::createSphere(float radius, uint32_t segments) {
    auto mesh = std::make_shared<Mesh>();
    
    std::vector<glm::vec3> positions;
    std::vector<uint32_t> indices;
    
    // Generate sphere vertices
    for (uint32_t i = 0; i <= segments; ++i) {
        float phi = 3.14159265359f * i / segments;
        for (uint32_t j = 0; j <= segments; ++j) {
            float theta = 2.0f * 3.14159265359f * j / segments;
            
            float x = radius * sin(phi) * cos(theta);
            float y = radius * cos(phi);
            float z = radius * sin(phi) * sin(theta);
            
            positions.push_back(glm::vec3(x, y, z));
        }
    }
    
    // Generate indices
    for (uint32_t i = 0; i < segments; ++i) {
        for (uint32_t j = 0; j < segments; ++j) {
            uint32_t first = i * (segments + 1) + j;
            uint32_t second = first + segments + 1;
            
            indices.push_back(first);
            indices.push_back(second);
            indices.push_back(first + 1);
            
            indices.push_back(second);
            indices.push_back(second + 1);
            indices.push_back(first + 1);
        }
    }
    
    mesh->createFromPositions(positions, indices);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::createPlane(float width, float height) {
    auto mesh = std::make_shared<Mesh>();
    
    std::vector<glm::vec3> positions = {
        {-width/2, 0, -height/2},
        { width/2, 0, -height/2},
        { width/2, 0,  height/2},
        {-width/2, 0,  height/2}
    };
    
    std::vector<uint32_t> indices = {
        0, 1, 2,
        0, 2, 3
    };
    
    mesh->createFromPositions(positions, indices);
    return mesh;
}

std::shared_ptr<Mesh> Mesh::createCylinder(float radius, float height, uint32_t segments) {
    // Basic implementation - would create cylinder geometry
    return createSphere(radius, segments);
}

std::shared_ptr<Mesh> Mesh::createCone(float radius, float height, uint32_t segments) {
    // Basic implementation - would create cone geometry
    return createSphere(radius, segments);
}

std::shared_ptr<Mesh> Mesh::createTorus(float outerRadius, float innerRadius, uint32_t segments, uint32_t sides) {
    // Basic implementation - would create torus geometry
    return createSphere(outerRadius, segments);
}

bool Mesh::isValid() const {
    return !vertices.empty() && loaded;
}

std::vector<std::string> Mesh::getValidationErrors() const {
    std::vector<std::string> errors;
    
    if (vertices.empty()) {
        errors.push_back("Mesh has no vertices");
    }
    
    if (!loaded) {
        errors.push_back("Mesh is not loaded");
    }
    
    return errors;
}

} // namespace nova
