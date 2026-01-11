#version 450

layout(std140, binding = 0) uniform UBO {
    ivec4 drawModes;
    vec4 color;

    mat4 matrix_model;
    mat4 matrix_view;
    mat4 matrix_projection;
    mat4 matrix_normals;
} ubo;

layout(std140, binding = 1) uniform UBO_DirLight {
    vec4 color;
    vec4 direction;
} ubo_dirLight;

layout(std140, binding = 2) uniform UBO_PointLight {
    vec4 color;
    vec4 position;
    vec4 attenuation; // [c, l, q, <unused>]
} ubo_pointLight;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;

layout(location = 0) out vec4 fragColor;

const float shininess = 32.0;
const float ambientStrength = 0.1;
const float specularStrength = 0.5;

void main()
{
    // -------------------------------
    // Transform vertex to view space
    // -------------------------------
    vec3 positionViewSpace = (ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0)).xyz;
    vec3 normalViewSpace   = normalize(mat3(ubo.matrix_normals) * inNormal);

    // Camera is at (0,0,0) in view space
    vec3 viewDirection = normalize(-positionViewSpace);

    // -------------------------------
    // Directional light (view space)
    // -------------------------------
    vec3 directionalLightDirection = normalize(-ubo_dirLight.direction.xyz);
    float directionalDiffuse = max(dot(normalViewSpace, directionalLightDirection), 0.0);
    vec3 directionalReflection = reflect(-directionalLightDirection, normalViewSpace);
    float directionalSpecular = pow(max(dot(viewDirection, directionalReflection), 0.0), shininess);

    vec3 directionalLighting =
        ambientStrength * ubo_dirLight.color.rgb +
        directionalDiffuse * ubo_dirLight.color.rgb +
        specularStrength * directionalSpecular * ubo_dirLight.color.rgb;

    // -------------------------------
    // Point light (view space)
    // -------------------------------
    vec3 pointLightVector = ubo_pointLight.position.xyz - positionViewSpace;
    float pointLightDistance = length(pointLightVector);
    vec3 pointLightDirection = normalize(pointLightVector);

    float pointDiffuse = max(dot(normalViewSpace, pointLightDirection), 0.0);
    vec3 pointReflection = reflect(-pointLightDirection, normalViewSpace);
    float pointSpecular = pow(max(dot(viewDirection, pointReflection), 0.0), shininess);

    float pointAttenuation =
        1.0 / (
            ubo_pointLight.attenuation.x +
            ubo_pointLight.attenuation.y * pointLightDistance +
            ubo_pointLight.attenuation.z * pointLightDistance * pointLightDistance
        );

    vec3 pointLighting =
        pointAttenuation * (
            ambientStrength * ubo_pointLight.color.rgb +
            pointDiffuse * ubo_pointLight.color.rgb +
            specularStrength * pointSpecular * ubo_pointLight.color.rgb
        );

    // -------------------------------
    // Final color (material * lighting)
    // -------------------------------
    vec3 lighting = directionalLighting + pointLighting;
    vec3 finalColor = inColor * lighting;

    fragColor = vec4(finalColor, 1.0);

    // -------------------------------
    // Final position
    // -------------------------------
    gl_Position = ubo.matrix_projection * vec4(positionViewSpace, 1.0);
}
