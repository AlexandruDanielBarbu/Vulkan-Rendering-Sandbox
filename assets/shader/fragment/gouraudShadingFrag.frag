#version 450

layout(std140, binding = 0) uniform UBO {
    ivec4 drawModes;
    vec4 material;

    mat4 matrix_model;
    mat4 matrix_view;
    mat4 matrix_projection;

    mat4 matrix_normals;
    mat4 view_inverse;
} ubo;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 outUV;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = fragColor;

    if (ubo.drawModes.z == 1) {
        outColor = vec4(outUV, 0, 1.0);
    }

}
