#version 450

layout(binding = 0) uniform ColorBuffer {
    vec4 color;
} uColor;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = uColor.color;
}
