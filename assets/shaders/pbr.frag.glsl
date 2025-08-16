#version 450
#include "pc_common.glsl"

layout(location=0) in vec3 vNrm;
layout(location=1) in vec2 vUV;
layout(location=2) in vec3 vWorldPos;
layout(location=3) in vec4 vShadowCoord; // Shadow coordinate for first light (for compatibility)
layout(location=0) out vec4 outColor;

// Uniform buffer for model matrix and light data
layout(set=0, binding=0) uniform UniformBufferObject {
    mat4 model;           // Model matrix
    vec4 lightPositions[3];  // 3 light positions
    vec4 lightColors[3];     // 3 light colors
    mat4 lightSpaceMatrices[3];   // Light space matrices for all lights
} ubo;

// Shadow maps for all lights (temporarily disabled)
// layout(set=0, binding=0) uniform sampler2DArrayShadow shadowMap2D;
// layout(set=0, binding=1) uniform sampler2DArrayShadow shadowMapCube;

float ShadowCalculation(vec3 worldPos, vec3 lightPos, float farPlane, int shadowMapIndex) {
    // Safety check for valid shadow map index
    if (shadowMapIndex < 0 || shadowMapIndex >= 3) {
        return 1.0; // Default to lit
    }
    
    // Calculate direction from light to fragment
    vec3 L = worldPos - lightPos;
    float dist = length(L);
    vec3 dir = L / max(dist, 1e-6);
    
    // Reference depth must be normalized to [0,1] over [near, far]
    float ref = dist / farPlane;
    
    // For now, just return lit (no shadows) until we implement proper shadow mapping
    // TODO: Implement proper shadow calculation with the new shadow system
    return 1.0; // Returns 0..1 (1 = lit)
}

void main(){
    vec3 N = normalize(vNrm);
    vec3 V = normalize(vec3(0.0, 0.0, 1.0));
    
    vec3 totalColor = vec3(0.0);
    
    // Process each light
    for (int i = 0; i < 3; i++) {
        // Skip if light has no intensity (safety check)
        if (length(ubo.lightColors[i].rgb) < 0.001) {
            continue;
        }
        
        // Calculate light direction from sphere position to light position
        vec3 L = normalize(ubo.lightPositions[i].xyz - vWorldPos);
        float NoL = max(dot(N, L), 0.0);
        
        // Calculate shadow for this light (point light with cubemap)
        float farPlane = 50.0; // Should match the far plane used in light space matrix calculation
        float shadow = ShadowCalculation(vWorldPos, ubo.lightPositions[i].xyz, farPlane, i);
        
        // Diffuse contribution (reduced by shadow)
        vec3 diffuse = PC.baseColor.rgb * NoL * ubo.lightColors[i].rgb * shadow;
        
        // Simple specular (also reduced by shadow)
        vec3 H = normalize(L + V);
        float NoH = max(dot(N, H), 0.0);
        float spec = pow(NoH, mix(8.0, 64.0, 1.0 - PC.roughness));
        vec3 specular = spec * 0.15 * ubo.lightColors[i].rgb * shadow;
        
        totalColor += diffuse + specular;
    }
    
    // Add ambient lighting to prevent completely dark areas
    vec3 ambient = PC.baseColor.rgb * 0.1;
    totalColor += ambient;
    
    outColor = vec4(totalColor, 1.0);
}
