#pragma once
#include <memory>
#include <string>
#include "engine/core/Camera.h"
#include "engine/core/LightingManager.h"
struct GLFWwindow;

namespace nova {
class VulkanRenderer;
class AssetManager;
class Camera;
class LightingManager;
class Mesh;
class Material;
class Texture;

class Editor {
public:
    void Init();
    void Run();
    void Shutdown();
    
private:
    void RenderUI();
    void DrawUI();
    void UpdateCursorMode(); // Update cursor mode based on ImGui state

private:
    void LoadDefaultAssets();
    
    GLFWwindow* m_window = nullptr;
    VulkanRenderer* m_renderer = nullptr;
    std::unique_ptr<Camera> m_camera;
    std::unique_ptr<LightingManager> m_lightingManager;
    float m_rotationAngle = 0.0f;
    double m_lastTime = 0.0;
    bool m_swapchainNeedsRecreation = false;
    bool m_cursorVisible = false; // Track cursor visibility state
    bool m_fullscreenToggleInProgress = false; // Prevent rapid fullscreen toggles
    
    // Asset System
    std::shared_ptr<AssetManager> m_assetManager;
    std::shared_ptr<Mesh> m_cubeMesh;
    std::shared_ptr<Mesh> m_sphereMesh;
    std::shared_ptr<Mesh> m_planeMesh;
    std::shared_ptr<Material> m_defaultMaterial;
    std::shared_ptr<Texture> m_defaultTexture;
    
    // Instance data for animated spheres
    std::vector<glm::mat4> m_instanceMatrices;
};

} // namespace nova
