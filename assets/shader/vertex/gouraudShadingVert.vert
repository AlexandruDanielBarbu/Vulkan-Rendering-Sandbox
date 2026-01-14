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

layout(location = 0) out vec4 fragColor;

vec3 computeDirectionalLighting(vec3 positionViewSpace, vec3 normalViewSpace, vec3 viewDirection) {
    vec3 directionalLightDirection = normalize(-ubo_dirLight.direction.xyz);
    float directionalDiffuse = max(dot(normalViewSpace, directionalLightDirection), 0.0);

    vec3 directionalReflection = reflect(-directionalLightDirection, normalViewSpace);
    float directionalSpecular = pow(max(dot(viewDirection, directionalReflection), 0.0), ubo.material.w);

    return
        ubo.material.x * ubo_dirLight.color.rgb +
        ubo.material.y * directionalDiffuse * ubo_dirLight.color.rgb +
        ubo.material.z * directionalSpecular * ubo_dirLight.color.rgb;

}

vec3 computePointLighting(vec3 positionViewSpace, vec3 normalViewSpace, vec3 viewDirection) {
    vec3 pointLightVector = ubo_pointLight.position.xyz - positionViewSpace;
    float pointLightDistance = length(pointLightVector);
    vec3 pointLightDirection = normalize(pointLightVector);

    float pointDiffuse = max(dot(normalViewSpace, pointLightDirection), 0.0);
    vec3 pointReflection = reflect(-pointLightDirection, normalViewSpace);
    float pointSpecular = pow(max(dot(viewDirection, pointReflection), 0.0), ubo.material.w);

    float pointAttenuation =
        1.0 / (
            ubo_pointLight.attenuation.x +
            ubo_pointLight.attenuation.y * pointLightDistance +
            ubo_pointLight.attenuation.z * pointLightDistance * pointLightDistance
        );

    return
        pointAttenuation * (
            ubo.material.x * ubo_pointLight.color.rgb +
            ubo.material.y * pointDiffuse * ubo_pointLight.color.rgb +
            ubo.material.z * pointSpecular * ubo_pointLight.color.rgb
        );
}

void main()
{
    // To view space transformations
    vec3 positionViewSpace = (ubo.matrix_view * ubo.matrix_model * vec4(inPosition, 1.0)).xyz;
    vec3 normalViewSpace   = normalize(mat3(ubo.matrix_normals) * inNormal);

    // Camera is at (0,0,0) in view space
    vec3 viewDirection = normalize(-positionViewSpace);

    // Directional light (view space)
    vec3 directionalLighting = computeDirectionalLighting(positionViewSpace, normalViewSpace, viewDirection);

    // Point light (view space)
    vec3 pointLighting = computePointLighting(positionViewSpace, normalViewSpace, viewDirection);

    // Final color (material * lighting)
    vec3 lighting = directionalLighting + pointLighting;
    vec3 finalColor = inColor * lighting;

    fragColor = vec4(finalColor, 1.0);

    // Show normals as color check
    if (ubo.drawModes.x == 1) {
        vec3 scaledNormal = 0.5 * normalViewSpace + 0.5;
        fragColor = vec4(
            pow(scaledNormal.x, 2.2),
            pow(scaledNormal.y, 2.2),
            pow(scaledNormal.z, 2.2),
            1
        );
    }

    gl_Position = ubo.matrix_projection * vec4(positionViewSpace, 1.0);
}
