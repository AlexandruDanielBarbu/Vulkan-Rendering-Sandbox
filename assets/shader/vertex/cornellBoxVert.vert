#version 450

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inUV;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPos;
layout(location = 2) out vec3 fragNormal;
layout(location = 3) out vec2 outUV;

layout(std140, binding = 0) uniform UBO {
    ivec4 drawModes;
    vec4 material;

    mat4 matrix_model;
    mat4 matrix_view;
    mat4 matrix_projection;

    mat4 matrix_normals;
    mat4 view_inverse;
} ubo;

void main() {
    fragColor  = inColor;
    fragPos    = (ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0)).xyz;
    fragNormal = normalize(mat3(ubo.matrix_normals) * inNormal);

    outUV = inUV;
    gl_Position = ubo.matrix_projection * ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0);
}
