#version 450
layout(location=0) out vec4 outColor;

void main() {
    // Output depth as color (for shadow mapping)
    // The depth is automatically written to the depth buffer
    outColor = vec4(1.0, 1.0, 1.0, 1.0);
}
