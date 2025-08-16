#version 450
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNrm;
layout(location=2) in vec2 inUV;
layout(location=3) in mat4 inInstanceMatrix;

layout(push_constant) uniform Push {
    mat4 uLightSpaceMatrix;
} pc;

void main() {
    vec4 worldPos = inInstanceMatrix * vec4(inPos, 1.0);
    gl_Position = pc.uLightSpaceMatrix * worldPos;
}
