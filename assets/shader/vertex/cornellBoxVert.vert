#version 450

layout(std140, binding = 0) uniform UBO {
    vec4 color;
    mat4 view_proj;
    mat4 normal_matrix;
    ivec4 draw_modes;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = ubo.view_proj * vec4(inPosition, 1.0);

    // Caped normals
    vec3 normalColor = normalize(inNormal);
    normalColor = normalColor * 0.5 + 0.5;

    if (ubo.draw_modes.x == 1) fragColor = vec4(normalColor, 0.0);
    else fragColor = vec4(inColor, 0.0);
    
    //fragNormal = normalize(mat3(ubo.normal_matrix) * inNormal);
    
}
