#version 450
#include "pc_common.glsl"

layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNrm;
layout(location=2) in vec2 inUV;
layout(location=3) in mat4 inInstanceMatrix;

layout(location=0) out vec3 vNrm;
layout(location=1) out vec2 vUV;
layout(location=2) out vec3 vWorldPos;
layout(location=3) out vec4 vShadowCoord; // Shadow coordinate for first light (for compatibility)

// Uniform buffer for model matrix and light data
layout(set=0, binding=0) uniform UniformBufferObject {
    mat4 model;           // Model matrix
    vec4 lightPositions[3];  // 3 light positions
    vec4 lightColors[3];     // 3 light colors
    mat4 lightSpaceMatrices[3];   // Light space matrices for all lights
} ubo;

void main() {
    vNrm = mat3(inInstanceMatrix) * inNrm;
    vUV = inUV;
    vec4 worldPos = inInstanceMatrix * vec4(inPos, 1.0);
    vWorldPos = worldPos.xyz; // Pass world position to fragment shader
    
    // Calculate shadow coordinate for first light (for compatibility)
    vShadowCoord = ubo.lightSpaceMatrices[0] * worldPos;
    
    // Use only the instance matrix transformation (ubo.model is not needed for instanced rendering)
    gl_Position = PC.viewProj * worldPos;
}
