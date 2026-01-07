#version 450

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragNormal;

layout(location = 0) out vec4 outColor;

void main() {
    // outColor = fragColor;
    outColor = vec4(fragNormal, 1.0);
}
