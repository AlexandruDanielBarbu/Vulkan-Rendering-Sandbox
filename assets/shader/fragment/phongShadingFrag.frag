#version 450

layout(location = 0) in vec3 fragPositionVS;
layout(location = 1) in vec3 fragNormalVS;
layout(location = 2) in vec3 fragColorVS;
layout(location = 3) in vec3 fragDirLightColor;
layout(location = 4) in vec3 fragDirLightDir;
layout(location = 5) in vec3 fragPointLightColor;
layout(location = 6) in vec3 fragPointLightPos;
layout(location = 7) in vec3 fragPointLightAttenuation;
layout(location = 8) in mat4 viewInverse;

layout(location = 0) out vec4 fragColor;

const float shininess = 32.0;
const float ambientStrength = 0.1;
const float specularStrength = 0.5;

// Brief - GCG utility function
//
// Gets the reflected color value from a certain position, from
// a certain direction INSIDE of a cornell box of size 3 which is
// positioned at the origin.
//
// positionWS:  Position inside the cornell box for which to get the
//              reflected color value for from -directionWS.
//
// directionWS: Outgoing direction vector (from positionWS towards the
//              outside) for which to get the reflected color value for.
//
// NOTE: view-space position!
//
vec3 getCornellBoxReflectionColor(vec3 positionWS, vec3 directionWS) {
    vec3 P0 = (viewInverse * vec4(positionWS, 1.0)).xyz;
    vec3 V  = normalize((viewInverse * vec4(directionWS, 0.0)).xyz);

    const float boxSize = 1.5;
    vec4[5] planes = {
        vec4(-1.0, 0.0, 0.0, -boxSize), // left
        vec4( 1.0, 0.0, 0.0, -boxSize), // right
        vec4( 0.0, 1.0, 0.0, -boxSize), // top
        vec4( 0.0, -1.0, 0.0, -boxSize), // bottom
        vec4( 0.0, 0.0, -1.0, -boxSize) // back
    };

    vec3[5] colors = {
        vec3(0.49, 0.06, 0.22), // left
        vec3(0.0, 0.13, 0.31), // right
        vec3(0.96, 0.93, 0.85), // top
        vec3(0.64, 0.64, 0.64), // bottom
        vec3(0.76, 0.74, 0.68) // back
    };

    for (int i = 0; i < 5; ++i) {
        vec3 N = planes[i].xyz;
        float d = planes[i].w;
        float denom = dot(V, N);
        
        if (denom <= 0) continue;
        
        float t = -(dot(P0, N) + d)/denom;
        vec3 P = P0 + t*V;
        float q = boxSize + 0.01;
        
        if (P.x > -q && P.x < q && P.y > -q && P.y < q && P.z > -q && P.z < q) {
            return colors[i];
        }
    }
    return vec3(0.0, 0.0, 0.0);
}

// Computes the reflection direction for an incident vector I about normal N,
// and clamps the reflection to a maximum of 180 degrees, i.e. the reflection vector
// will always lie within the hemisphere around normal N. Aside from clamping,
// this function produces the same result as GLSL's reflect function.
vec3 clampedReflect(vec3 I, vec3 N) {
    return I - 2.0 * min(dot(N, I), 0.0) * N;
}

const float F0 = 0.1;
float fresnelFactor(float cosTheta) {
    return F0 + (1 - F0) * pow((1 - cosTheta), 5);
}

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

        // Get Fresnel value
        float cosTheta = clamp(dot(normal, viewDir), 0.0, 1.0);
        float fresnel = fresnelFactor(cosTheta);
        
        // get reflection
        vec3 reflection = clampedReflect(-viewDir, normal);
        // get reflection on cornel box
        vec3 cornelReflection = getCornellBoxReflectionColor(fragPositionVS, reflection);
        // mix values
        vec3 mixedColor = mix(finalColor, cornelReflection, fresnel);
        // final color
        fragColor = vec4(mixedColor, 1.0);


    //fragColor = vec4(finalColor, 1.0);
}
