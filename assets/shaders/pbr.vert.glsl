#version 450
layout(location=0) in vec3 inPos;
layout(location=1) in vec3 inNrm;
layout(location=2) in vec2 inUV;

layout(location=0) out vec3 vNrm;
layout(location=1) out vec2 vUV;

layout(push_constant) uniform Push {
    mat4 uMVP;
    vec4 uBaseColor;
    float uMetallic;
    float uRoughness;
} pc;

void main() {
    vNrm = inNrm;
    vUV = inUV;
    gl_Position = pc.uMVP * vec4(inPos,1.0);
}
