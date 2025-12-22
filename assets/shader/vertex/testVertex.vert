#version 450

layout(std140, binding = 0) uniform UBO {
    vec4 color;
    mat4 view_proj;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = ubo.view_proj * vec4(inPosition, 1.0);

    fragColor = vec4(inColor, 0.0);
}
