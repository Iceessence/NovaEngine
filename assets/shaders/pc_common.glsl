// Push constants (≤256 bytes for best compatibility)
layout(push_constant) uniform PushConstants {
    mat4 viewProj;   // 64 bytes - view * projection matrix
    vec4 baseColor;  // 16 bytes - material base color
    float metallic;  // 4 bytes - material metallic factor
    float roughness; // 4 bytes - material roughness factor
    // Total: 88 bytes (well under 256 byte limit)
} PC;
