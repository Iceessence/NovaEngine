#pragma once
#include <glm/glm.hpp>
#include <string>
namespace nova {
struct RenderStats { float frameTimeMs=0; };
class IRenderer {
public:
    virtual ~IRenderer() = default;
    virtual bool Init(void* glfwWindowHandle) = 0;
    virtual void Resize(int w,int h) = 0;
    virtual void BeginFrame(const glm::mat4& viewProj) = 0;
    virtual void DrawCube(const glm::mat4& model, const glm::vec3& baseColor, float metallic, float roughness) = 0;
    virtual void EndFrame() = 0;
    virtual RenderStats Stats() const = 0;
};
}
