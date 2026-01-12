#version 450

layout(std140, binding = 0) uniform UBO {
    ivec4 drawModes;
    vec4 color;

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
layout(location = 2) out vec3 fragColorVS;
layout(location = 3) out vec3 fragDirLightColor;
layout(location = 4) out vec3 fragDirLightDir;
layout(location = 5) out vec3 fragPointLightColor;
layout(location = 6) out vec3 fragPointLightPos;
layout(location = 7) out vec3 fragPointLightAttenuation;
layout(location = 8) out mat4 viewInverse;

void main()
{
    // Transform vertex to view space
    vec3 positionVS = (ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0)).xyz;
    vec3 normalVS   = normalize(mat3(ubo.matrix_normals) * inNormal);

    // Pass everything needed to fragment shader
    fragPositionVS = positionVS;
    fragNormalVS   = normalVS;
    fragColorVS    = inColor;

    fragDirLightColor = ubo_dirLight.color.rgb;
    fragDirLightDir   = normalize(-ubo_dirLight.direction.xyz);

    fragPointLightColor       = ubo_pointLight.color.rgb;
    fragPointLightPos         = ubo_pointLight.position.xyz;
    fragPointLightAttenuation = ubo_pointLight.attenuation.xyz;

    viewInverse = ubo.view_inverse;

    gl_Position = ubo.matrix_projection * vec4(positionVS, 1.0);
}
