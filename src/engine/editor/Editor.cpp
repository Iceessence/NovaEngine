#include "Editor.h"
#include "engine/core/Log.h"
#include "engine/renderer/vk/VulkanRenderer.h"
#include "engine/assets/AssetManager.h"
#include "engine/assets/Texture.h"
#include "engine/assets/Material.h"
#include "engine/assets/Mesh.h"
#include "engine/assets/importers/GLTFImporter.h"
#include "engine/core/Camera.h"
#include "engine/core/LightingManager.h"
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <thread>
#include <chrono>

namespace nova {

void Editor::Init() {
    NOVA_INFO("Editor::Init");
    if (!glfwInit()) {
        NOVA_INFO("GLFW init failed");
        throw std::runtime_error("Failed to initialize GLFW");
    }
    
    // Create window with Vulkan support
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    m_window = glfwCreateWindow(1280, 720, "NovaEngine - Asset System Demo", nullptr, nullptr);
    if (!m_window) {
        NOVA_INFO("GLFW create window failed");
        throw std::runtime_error("Failed to create GLFW window");
    }
    NOVA_INFO("GLFW window created (1280x720)");
    
    // Make sure window is visible and positioned
    glfwShowWindow(m_window);
    glfwFocusWindow(m_window);
    NOVA_INFO("GLFW window shown and focused");
    
    // Initialize Vulkan renderer
    m_renderer = new VulkanRenderer();
    try {
        m_renderer->Init(m_window);
        m_renderer->InitImGui(m_window);
    } catch (const std::exception& e) {
        NOVA_ERROR("Failed to initialize Vulkan renderer: " + std::string(e.what()));
        throw;
    }
    
    // Initialize Asset Manager
    try {
        m_assetManager = std::make_shared<AssetManager>();
        NOVA_INFO("Asset Manager initialized");
    } catch (const std::exception& e) {
        NOVA_ERROR("Failed to initialize Asset Manager: " + std::string(e.what()));
        throw;
    }
    
    // Initialize Camera
    m_camera = std::make_unique<Camera>();
    m_camera->SetPosition(glm::vec3(0.0f, 0.0f, 8.0f));
    m_camera->SetAspectRatio(1280.0f / 720.0f);
    NOVA_INFO("Camera initialized");
    
    // Initialize Lighting Manager
    m_lightingManager = std::make_unique<LightingManager>();
    m_lightingManager->SetupThreePointLighting(); // Start with three-point lighting
    NOVA_INFO("Lighting Manager initialized with " + std::to_string(m_lightingManager->GetLightCount()) + " lights");
    
    // Initialize lights in renderer
    m_renderer->SetLightsFromManager(m_lightingManager.get());
    
    // Set up mouse input callbacks
    glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    m_cursorVisible = false; // Start with cursor hidden
    glfwSetWindowUserPointer(m_window, this);
    
    // Mouse callback - chain to ImGui first, then handle camera
    glfwSetCursorPosCallback(m_window, [](GLFWwindow* window, double xpos, double ypos) {
        // Always forward to ImGui first
        ImGui_ImplGlfw_CursorPosCallback(window, xpos, ypos);
        
        Editor* editor = static_cast<Editor*>(glfwGetWindowUserPointer(window));
        static double lastX = xpos;
        static double lastY = ypos;
        static bool firstMouse = true;
        
        // Only process camera movement if ImGui doesn't want the mouse and cursor is hidden
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && !editor->m_cursorVisible) {
            if (firstMouse) {
                lastX = xpos;
                lastY = ypos;
                firstMouse = false;
            }
            
            float xoffset = xpos - lastX;
            float yoffset = lastY - ypos;
            lastX = xpos;
            lastY = ypos;
            
            editor->m_camera->ProcessMouseMovement(xoffset, yoffset);
        } else {
            // Reset first mouse flag when cursor becomes visible again
            firstMouse = true;
        }
    });
    
    // Scroll callback - chain to ImGui first, then handle camera
    glfwSetScrollCallback(m_window, [](GLFWwindow* window, double xoffset, double yoffset) {
        // Always forward to ImGui first
        ImGui_ImplGlfw_ScrollCallback(window, xoffset, yoffset);
        
        // Only process camera scroll if ImGui doesn't want it and cursor is hidden
        Editor* editor = static_cast<Editor*>(glfwGetWindowUserPointer(window));
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureMouse && !editor->m_cursorVisible) {
            editor->m_camera->ProcessMouseScroll(yoffset);
        }
    });
    
    // Mouse button callback - chain to ImGui first
    glfwSetMouseButtonCallback(m_window, [](GLFWwindow* window, int button, int action, int mods) {
        // Always forward to ImGui first
        ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
        
        // Add any custom mouse button handling here if needed
    });
    
    // Key callback - chain to ImGui first
    glfwSetKeyCallback(m_window, [](GLFWwindow* window, int key, int scancode, int action, int mods) {
        // Always forward to ImGui first
        ImGui_ImplGlfw_KeyCallback(window, key, scancode, action, mods);
        
        // Add any custom key handling here if needed
    });
    
    // Char callback - chain to ImGui first
    glfwSetCharCallback(m_window, [](GLFWwindow* window, unsigned int c) {
        // Always forward to ImGui first
        ImGui_ImplGlfw_CharCallback(window, c);
    });
    
    // Window resize callback with proper ImGui frame management
    glfwSetFramebufferSizeCallback(m_window, [](GLFWwindow* window, int width, int height) {
        Editor* editor = static_cast<Editor*>(glfwGetWindowUserPointer(window));
        if (editor && editor->m_renderer && width > 0 && height > 0) {
            // Signal that the swapchain needs to be recreated
            editor->m_swapchainNeedsRecreation = true;
            NOVA_INFO("Window resized to " + std::to_string(width) + "x" + std::to_string(height));
        }
    });
    
    // Load default assets to demonstrate the system
    NOVA_INFO("About to call LoadDefaultAssets()");
    LoadDefaultAssets();
    NOVA_INFO("LoadDefaultAssets() completed");
    
    // Display cursor control information
    NOVA_INFO("=== Cursor Controls ===");
    NOVA_INFO("TAB - Toggle cursor visibility");
    NOVA_INFO("F11 - Toggle fullscreen");
    NOVA_INFO("Cursor starts hidden for camera control");
    NOVA_INFO("Cursor automatically shows when interacting with UI");
    NOVA_INFO("=======================");
    
    // Test GLTF importer with sphere
    NOVA_INFO("Testing GLTF importer with sphere...");
    
    auto gltfImporter = std::make_unique<GLTFImporter>(m_assetManager);
    
    GLTFImportResult result = gltfImporter->importFromFile("Assets/Meshes/sphere.gltf");
    
    if (result.success && !result.meshes.empty()) {
        NOVA_INFO("GLTF import successful! Using sphere mesh.");
        
        auto sphereMesh = result.meshes[0];
        auto vertexData = sphereMesh->getVertexDataForRenderer();
        auto indexData = sphereMesh->getIndexDataForRenderer();
        
        NOVA_INFO("Sphere mesh data: " + std::to_string(vertexData.size() / 8) + " vertices, " + std::to_string(indexData.size()) + " indices");
        
        m_renderer->SetAssetData(vertexData, indexData);
        NOVA_INFO("Sphere mesh data set in renderer");
        
        // Create multiple instances for GPU instancing demo
        NOVA_INFO("Creating multiple sphere instances...");
        std::vector<glm::mat4> instanceMatrices;
        
        // Create a 3x3x3 grid of spheres with better spacing
        for (int x = -1; x <= 1; ++x) {
            for (int y = -1; y <= 1; ++y) {
                for (int z = -1; z <= 1; ++z) {
                    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(x * 4.0f, y * 4.0f, z * 4.0f));
                    model = glm::scale(model, glm::vec3(1.0f)); // Make spheres full size
                    instanceMatrices.push_back(model);
                }
            }
        }
        
        // Store instance matrices for animation
        m_instanceMatrices = instanceMatrices;
        
        m_renderer->SetInstanceData(instanceMatrices);
        NOVA_INFO("Created " + std::to_string(instanceMatrices.size()) + " sphere instances");
        
        // Debug: Log the first few instance positions
        for (int i = 0; i < std::min(5, (int)instanceMatrices.size()); ++i) {
            glm::vec3 pos = glm::vec3(instanceMatrices[i][3]);
            NOVA_INFO("Instance " + std::to_string(i) + " position: (" + 
                     std::to_string(pos.x) + ", " + std::to_string(pos.y) + ", " + std::to_string(pos.z) + ")");
        }
        
    } else {
        NOVA_INFO("GLTF import failed, using fallback cube data.");
        
        // Fallback to cube data
        std::vector<float> hardcodedVertexData = {
            // Front face
            -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
            
            // Back face
            -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
             0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
            
            // Left face
            -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
            
            // Right face
             0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
             0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
            
            // Bottom face
            -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
             0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
             0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
            
            // Top face
            -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
             0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f
        };
        
        std::vector<uint32_t> hardcodedIndexData = {
            0,  1,  2,    2,  3,  0,   // front
            4,  5,  6,    6,  7,  4,   // back
            8,  9,  10,   10, 11, 8,   // left
            12, 13, 14,   14, 15, 12,  // right
            16, 17, 18,   18, 19, 16,  // bottom
            20, 21, 22,   22, 23, 20   // top
        };
        
        m_renderer->SetAssetData(hardcodedVertexData, hardcodedIndexData);
        NOVA_INFO("Fallback cube data set in renderer");
    }
    
    m_lastTime = glfwGetTime();
    NOVA_INFO("Editor initialized successfully");
}

void Editor::LoadDefaultAssets() {
    NOVA_INFO("Loading default assets...");
    
    try {
        NOVA_INFO("Starting asset registration...");
        
        // Register and load a default cube mesh
        NOVA_INFO("Registering cube mesh...");
        AssetGUID cubeGUID = m_assetManager->registerAsset("builtin:cube", AssetType::Mesh);
        NOVA_INFO("Cube mesh registered with GUID: " + cubeGUID);
        if (!cubeGUID.empty()) {
            NOVA_INFO("Loading cube mesh...");
            bool success = m_assetManager->loadAsset(cubeGUID);
            NOVA_INFO("Cube mesh load result: " + std::string(success ? "SUCCESS" : "FAILED"));
            if (success) {
                NOVA_INFO("Getting cube mesh from manager...");
                m_cubeMesh = m_assetManager->getMesh(cubeGUID);
                NOVA_INFO("Loaded cube mesh: " + cubeGUID);
            } else {
                NOVA_INFO("Failed to load cube mesh: " + cubeGUID);
            }
        }
        
        // Register and load a default material
        AssetGUID materialGUID = m_assetManager->registerAsset("builtin:default_material", AssetType::Material);
        if (!materialGUID.empty()) {
            bool success = m_assetManager->loadAsset(materialGUID);
            if (success) {
                m_defaultMaterial = m_assetManager->getMaterial(materialGUID);
                NOVA_INFO("Loaded default material: " + materialGUID);
            } else {
                NOVA_INFO("Failed to load default material: " + materialGUID);
            }
        }
        
        // Register and load a default texture
        AssetGUID textureGUID = m_assetManager->registerAsset("builtin:default_texture", AssetType::Texture);
        if (!textureGUID.empty()) {
            bool success = m_assetManager->loadAsset(textureGUID);
            if (success) {
                m_defaultTexture = m_assetManager->getTexture(textureGUID);
                NOVA_INFO("Loaded default texture: " + textureGUID);
            } else {
                NOVA_INFO("Failed to load default texture: " + textureGUID);
            }
        }
        
        // Demonstrate dependency tracking
        NOVA_INFO("Setting up asset dependencies...");
        if (!materialGUID.empty() && !textureGUID.empty()) {
            m_assetManager->addDependency(materialGUID, textureGUID);
            NOVA_INFO("Added dependency: Material -> Texture");
        }
        
        if (!cubeGUID.empty() && !materialGUID.empty()) {
            m_assetManager->addDependency(cubeGUID, materialGUID);
            NOVA_INFO("Added dependency: Mesh -> Material");
        }
        
        // Load asset database if it exists
        NOVA_INFO("Loading asset database...");
        m_assetManager->loadAssetDB();
        
        // Scan assets directory for additional files
        NOVA_INFO("Scanning assets directory...");
        m_assetManager->scanAssetsDirectory();
        
        // Set up hot-reload callback
        m_assetManager->setAssetChangedCallback([](const std::string& path) {
            NOVA_INFO("Asset changed, hot-reload triggered: " + path);
        });
        
        NOVA_INFO("Default assets loading completed");
    } catch (const std::exception& e) {
        NOVA_INFO("Exception during asset loading: " + std::string(e.what()));
    } catch (...) {
        NOVA_INFO("Unknown exception during asset loading");
    }
}

void Editor::UpdateCursorMode() {
    ImGuiIO& io = ImGui::GetIO();
    const bool uiWantsMouse = io.WantCaptureMouse;

    // If UI wants the mouse, always show it
    if (uiWantsMouse) {
        if (!m_cursorVisible) {
            m_cursorVisible = true;
            glfwSetInputMode(m_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            NOVA_INFO("Cursor shown for UI interaction");
        }
        return;
    }

    // Otherwise respect the camera look toggle
    // (The cursor visibility is managed by TAB key in the main loop)
}

void Editor::Run() {
    NOVA_INFO("Editor::Run — entering loop");
    
    try {
        // Set up error callback to catch any GLFW errors
        glfwSetErrorCallback([](int error, const char* description) {
            NOVA_INFO("GLFW Error " + std::to_string(error) + ": " + std::string(description));
        });
        
        // Set up window close callback to see what's requesting the close
        glfwSetWindowCloseCallback(m_window, [](GLFWwindow* window) {
            NOVA_ERROR("GLFW Window Close Callback triggered - this should NOT happen automatically!");
            // Allow normal window closing behavior
        });
        
        // Set up window focus callback to track focus changes
        glfwSetWindowFocusCallback(m_window, [](GLFWwindow* window, int focused) {
            NOVA_INFO("GLFW Window Focus Callback: " + std::string(focused ? "Gained focus" : "Lost focus"));
            if (!focused) {
                NOVA_ERROR("Window lost focus - this might be causing issues");
            }
        });

        int frameCount = 0;
        while (m_window && !glfwWindowShouldClose(m_window)) {
            NOVA_INFO("Editor::Run: Loop condition check - m_window: " + std::to_string(m_window != nullptr) + ", shouldClose: " + std::to_string(glfwWindowShouldClose(m_window)));
            frameCount++;
            NOVA_INFO("Editor::Run: Loop iteration start - Frame " + std::to_string(frameCount));
            glfwPollEvents();
            
                    // Debug: Check if window should close
        if (glfwWindowShouldClose(m_window)) {
            NOVA_INFO("Editor::Run: Window should close detected");
            // Allow normal window closing behavior
        }
        
        // Begin ImGui frame for GLFW
        if (m_renderer->IsImGuiReady()) {
            ImGui_ImplGlfw_NewFrame();
        }
        
        // Update cursor mode based on ImGui state
        UpdateCursorMode();
        
        // Easy exit controls - ESC or Q to exit (check ImGui first)
        ImGuiIO& io = ImGui::GetIO();
        if (!io.WantCaptureKeyboard && 
            (glfwGetKey(m_window, GLFW_KEY_ESCAPE) == GLFW_PRESS || 
             glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS)) {
            NOVA_INFO("Editor::Run: Exit key (ESC or Q) pressed");
            break;
        }
        
        // Cursor toggle - Tab key (only when ImGui doesn't want keyboard)
        static bool tabPressed = false;
        if (!io.WantCaptureKeyboard && glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_PRESS && !tabPressed) {
            m_cursorVisible = !m_cursorVisible;
            glfwSetInputMode(m_window, GLFW_CURSOR, 
                m_cursorVisible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
            NOVA_INFO("Cursor " + std::string(m_cursorVisible ? "shown" : "hidden"));
            tabPressed = true;
        } else if (glfwGetKey(m_window, GLFW_KEY_TAB) == GLFW_RELEASE) {
            tabPressed = false;
        }
        
        // Fullscreen toggle - F11 key (only when ImGui doesn't want keyboard)
        static bool f11Pressed = false;
        if (!io.WantCaptureKeyboard && glfwGetKey(m_window, GLFW_KEY_F11) == GLFW_PRESS && !f11Pressed) {
            NOVA_INFO("F11 pressed - attempting fullscreen toggle");
            try {
                m_renderer->ToggleFullscreen();
                NOVA_INFO("Fullscreen toggle completed");
            } catch (const std::exception& e) {
                NOVA_INFO("Fullscreen toggle failed: " + std::string(e.what()));
                // Don't break for debugging - continue running
            } catch (...) {
                NOVA_INFO("Fullscreen toggle failed with unknown error");
                // Don't break for debugging - continue running
            }
            f11Pressed = true;
        } else if (glfwGetKey(m_window, GLFW_KEY_F11) == GLFW_RELEASE) {
            f11Pressed = false;
        }
        
        // Calculate delta time
        double currentTime = glfwGetTime();
        double deltaTime = currentTime - m_lastTime;
        m_lastTime = currentTime;
        
        // Update camera only when cursor is hidden and ImGui doesn't want input
        if (!m_cursorVisible && !io.WantCaptureKeyboard) {
            m_camera->Update(static_cast<float>(deltaTime), m_window);
        }
        
        // Note: Removed automatic light animation - lights only change when manually adjusted in UI
        // m_lightingManager->UpdateLights(static_cast<float>(deltaTime));
        // m_renderer->SetLightsFromManager(m_lightingManager.get());
        
        // Simple rotation for testing
        m_rotationAngle += 60.0f * static_cast<float>(deltaTime);
        if (m_rotationAngle > 360.0f) {
            m_rotationAngle -= 360.0f;
        }

        // Animate the spheres - make them rotate around their positions
        std::vector<glm::mat4> animatedInstances = m_instanceMatrices;
        for (size_t i = 0; i < animatedInstances.size(); ++i) {
            // Get the original position
            glm::vec3 originalPos = glm::vec3(animatedInstances[i][3]);
            
            // Create a rotation matrix that varies by instance
            float instanceRotation = m_rotationAngle + (i * 30.0f); // Each sphere rotates at different speed
            glm::mat4 rotation = glm::rotate(glm::mat4(1.0f), glm::radians(instanceRotation), glm::vec3(0.0f, 1.0f, 0.0f));
            
            // Add some bobbing motion
            float bobHeight = sin(glm::radians(m_rotationAngle * 2.0f + i * 45.0f)) * 0.5f;
            glm::vec3 animatedPos = originalPos + glm::vec3(0.0f, bobHeight, 0.0f);
            
            // Combine rotation and translation
            animatedInstances[i] = glm::translate(glm::mat4(1.0f), animatedPos) * rotation * glm::scale(glm::mat4(1.0f), glm::vec3(1.0f));
        }
        
        // Update the renderer with animated instances
        m_renderer->SetInstanceData(animatedInstances);

        // Create simple MVP matrix (not used for instanced rendering, but kept for compatibility)
        glm::mat4 model = glm::rotate(glm::mat4(1.0f), glm::radians(m_rotationAngle), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 view = m_camera->GetViewMatrix();
        glm::mat4 projection = m_camera->GetProjectionMatrix();
        glm::mat4 mvp = projection * view * model;
        
        // Update renderer with MVP matrix
        m_renderer->UpdateMVP(mvp);
        
        // Update performance metrics
        m_renderer->UpdatePerformanceMetrics(deltaTime);
        
        
        

        
        // Handle swapchain recreation for fullscreen support
        if (m_swapchainNeedsRecreation) {
            try {
                NOVA_INFO("Starting swapchain recreation...");
                m_renderer->RecreateSwapchain();
                m_swapchainNeedsRecreation = false;
                NOVA_INFO("Swapchain recreated successfully");
            } catch (const std::exception& e) {
                NOVA_INFO("Error during swapchain recreation: " + std::string(e.what()));
                m_swapchainNeedsRecreation = false;
                // Don't break for debugging - continue running
            } catch (...) {
                NOVA_INFO("Unknown error during swapchain recreation");
                m_swapchainNeedsRecreation = false;
                // Don't break for debugging - continue running
            }
        }
        

        
        // Render the frame (minimal version)
        NOVA_INFO("Editor::Run: About to call RenderFrame");
        try {
            if (m_renderer) {
                NOVA_INFO("Editor::Run: Calling RenderFrame...");
                m_renderer->RenderFrame(m_camera.get(), m_lightingManager.get());
                NOVA_INFO("Editor::Run: RenderFrame completed successfully");
                NOVA_INFO("Editor::Run: After RenderFrame call");
            } else {
                NOVA_ERROR("Renderer is null, skipping frame");
            }
        } catch (const std::exception& e) {
            NOVA_ERROR("Error in RenderFrame: " + std::string(e.what()));
            // Continue running instead of breaking
        } catch (...) {
            NOVA_ERROR("Unknown error in RenderFrame");
            // Continue running instead of breaking
        }
        
        // Small delay to prevent excessive CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(16)); // ~60 FPS
        
        // Update window title to show it's running
        std::string title = "NovaEngine - Frame: " + std::to_string(frameCount);
        glfwSetWindowTitle(m_window, title.c_str());
        
        NOVA_INFO("Editor::Run: Loop iteration end - Frame: " + std::to_string(frameCount));
        NOVA_INFO("Editor::Run: About to check window close status...");
        
        // Check if window should close
        if (glfwWindowShouldClose(m_window)) {
            NOVA_INFO("Editor::Run: Window should close detected - BREAKING LOOP");
            break;
        } else {
            NOVA_INFO("Editor::Run: Window should NOT close - continuing");
        }
        
        // Additional debugging - check window state
        if (glfwGetWindowAttrib(m_window, GLFW_VISIBLE) == GLFW_FALSE) {
            NOVA_INFO("Editor::Run: Window is not visible - forcing to stay visible");
            glfwShowWindow(m_window);
        }
        
        if (glfwGetWindowAttrib(m_window, GLFW_ICONIFIED) == GLFW_TRUE) {
            NOVA_INFO("Editor::Run: Window is minimized - continuing");
        }
        
        // Force a longer delay to see if the application is actually running
        if (frameCount < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(500)); // 0.5 second delay
            NOVA_INFO("Editor::Run: Extended delay for frame " + std::to_string(frameCount));
        }
        
        // Allow normal application flow
        }

        NOVA_INFO("Editor::Run — leaving loop");
        NOVA_INFO("Editor::Run — about to call Shutdown()");
    } catch (const std::exception& e) {
        NOVA_ERROR("Editor::Run: Exception caught: " + std::string(e.what()));
        NOVA_ERROR("Editor::Run: Exception type: " + std::string(typeid(e).name()));
    } catch (...) {
        NOVA_ERROR("Editor::Run: Unknown exception caught");
    }
}

void Editor::Shutdown() {
    NOVA_INFO("Editor::Shutdown: Starting shutdown process");
    
    // Clean up asset manager
    NOVA_INFO("Editor::Shutdown: Cleaning up asset manager");
    m_assetManager.reset();
    NOVA_INFO("Editor::Shutdown: Asset manager cleaned up");
    
    if (m_renderer) {
        NOVA_INFO("Editor::Shutdown: Shutting down renderer");
        m_renderer->Shutdown();
        NOVA_INFO("Editor::Shutdown: Renderer shut down, deleting renderer");
        delete m_renderer;
        m_renderer = nullptr;
        NOVA_INFO("Editor::Shutdown: Renderer deleted");
    }
    
    if (m_window) { 
        NOVA_INFO("Editor::Shutdown: Destroying GLFW window");
        glfwDestroyWindow(m_window); 
        m_window = nullptr; 
        NOVA_INFO("Editor::Shutdown: GLFW window destroyed");
    }
    NOVA_INFO("Editor::Shutdown: Terminating GLFW");
    glfwTerminate();
    NOVA_INFO("Editor::Shutdown: GLFW terminated");
    NOVA_INFO("Editor::Shutdown: Complete");
}

void Editor::RenderUI() {
    // UI rendering is handled by VulkanRenderer
}

} // namespace nova
