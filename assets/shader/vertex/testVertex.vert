#version 450

layout(std140, binding = 0) uniform UBO {
        ivec4 drawModes;
        
        vec4 color;
        
        mat4 matrix_model;
        mat4 matrix_view;
        mat4 matrix_projection;

        mat4 matrix_normals;
} ubo;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

void main() {
    gl_Position = ubo.matrix_projection * ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0);

    // Caped normals
    vec3 normalVS = normalize(mat3(ubo.matrix_normals) * inNormal);
    vec3 normalColor = normalVS * 0.5 + 0.5;

    if (ubo.drawModes.x == 1) fragColor = vec4(normalColor, 0.0);
    else fragColor = vec4(inColor, 0.0);
    
    //fragNormal = normalize(mat3(ubo.normal_matrix) * inNormal);
    
}
