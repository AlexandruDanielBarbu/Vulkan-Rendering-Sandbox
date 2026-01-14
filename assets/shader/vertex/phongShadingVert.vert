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

layout(std140, binding = 1) uniform UBO_DirLight {
    vec4 color;
    vec4 direction;
} ubo_dirLight;

layout(std140, binding = 2) uniform UBO_PointLight {
    vec4 color;
    vec4 position;
    vec4 attenuation;
} ubo_pointLight;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec3 fragPositionVS;
layout(location = 1) out vec3 fragNormalVS;
layout(location = 2) out vec3 fragColor;
layout(location = 3) out vec3 normalColor;

void main()
{
    // Transform vertex to view space
    fragPositionVS = (ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0)).xyz;
    fragNormalVS   = normalize(mat3(ubo.matrix_normals) * inNormal);
    fragColor = inColor;

    // Get normalisez and scaled normal to display it as a color
    vec3 scaledNormal = 0.5 * fragNormalVS + 0.5;
    normalColor = vec3(
            pow(scaledNormal.x, 2.2),
            pow(scaledNormal.y, 2.2),
            pow(scaledNormal.z, 2.2)
    );

    gl_Position = ubo.matrix_projection * ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0);
}
