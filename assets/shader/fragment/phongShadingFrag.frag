#version 450

layout(location = 0) in vec3 fragPositionVS;
layout(location = 1) in vec3 fragNormalVS;
layout(location = 2) in vec3 fragColorVS;
layout(location = 3) in vec3 fragDirLightColor;
layout(location = 4) in vec3 fragDirLightDir;
layout(location = 5) in vec3 fragPointLightColor;
layout(location = 6) in vec3 fragPointLightPos;
layout(location = 7) in vec3 fragPointLightAttenuation;

layout(location = 0) out vec4 fragColor;

const float shininess = 32.0;
const float ambientStrength = 0.1;
const float specularStrength = 0.5;

void main()
{
    vec3 normal = normalize(fragNormalVS);
    vec3 viewDir = normalize(-fragPositionVS); // camera at origin

    // -------- Directional light --------
    float diffDir = max(dot(normal, fragDirLightDir), 0.0);
    vec3 reflectDir = reflect(-fragDirLightDir, normal);
    float specDir = pow(max(dot(viewDir, reflectDir), 0.0), shininess);

    vec3 directionalLighting = ambientStrength * fragDirLightColor +
                               diffDir * fragDirLightColor +
                               specularStrength * specDir * fragDirLightColor;

    // -------- Point light --------
    vec3 pointVec = fragPointLightPos - fragPositionVS;
    float distance = length(pointVec);
    vec3 pointDir = normalize(pointVec);

    float diffPoint = max(dot(normal, pointDir), 0.0);
    vec3 reflectPoint = reflect(-pointDir, normal);
    float specPoint = pow(max(dot(viewDir, reflectPoint), 0.0), shininess);

    float attenuation = 1.0 / (
        fragPointLightAttenuation.x +
        fragPointLightAttenuation.y * distance +
        fragPointLightAttenuation.z * distance * distance
    );

    vec3 pointLighting = attenuation * (ambientStrength * fragPointLightColor +
                                        diffPoint * fragPointLightColor +
                                        specularStrength * specPoint * fragPointLightColor);

    // -------- Final color --------
    vec3 lighting = directionalLighting + pointLighting;
    vec3 finalColor = fragColorVS * lighting;

    fragColor = vec4(finalColor, 1.0);
}
