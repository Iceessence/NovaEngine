#version 450
layout(location=0) in vec3 vNrm;
layout(location=1) in vec2 vUV;
layout(location=0) out vec4 outColor;

layout(push_constant) uniform Push {
    mat4 uMVP;
    vec4 uBaseColor;
    float uMetallic;
    float uRoughness;
} pc;

void main(){
    vec3 N = normalize(vNrm);
    vec3 L = normalize(vec3(0.4, 1.0, 0.2));
    vec3 V = normalize(vec3(0.0, 0.0, 1.0));
    float NoL = max(dot(N,L), 0.0);
    vec3 diffuse = pc.uBaseColor.rgb * NoL;
    // simple specular
    vec3 H = normalize(L+V);
    float NoH = max(dot(N,H), 0.0);
    float spec = pow(NoH, mix(8.0, 64.0, 1.0-pc.uRoughness));
    vec3 color = diffuse + spec * 0.15;
    outColor = vec4(color, 1.0);
}
