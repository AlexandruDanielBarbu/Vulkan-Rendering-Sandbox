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
    vec4 attenuation; // [c, l, q, <unused>]
} ubo_pointLight;

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
    vec3 P0 = (ubo.view_inverse * vec4(positionWS, 1.0)).xyz;
    vec3 V  = normalize((ubo.view_inverse * vec4(directionWS, 0.0)).xyz);

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

layout(location = 0) in vec3 fragPositionVS;
layout(location = 1) in vec3 fragNormalVS;
layout(location = 2) in vec3 fragColor;
layout(location = 3) in vec3 normalColor;
layout(location = 4) in vec2 uvCoordinates;

layout(location = 0) out vec4 fragColorOut;

void main()
{
    // To view space transformations
    vec3 positionViewSpace = fragPositionVS;
    vec3 normalViewSpace   = normalize(fragNormalVS);

    // Camera is at (0,0,0) in view space
    vec3 viewDirection = normalize(-positionViewSpace);

    // Directional light (view space)
    vec3 directionalLighting = computeDirectionalLighting(positionViewSpace, normalViewSpace, viewDirection);

    // Point light (view space)
    vec3 pointLighting = computePointLighting(positionViewSpace, normalViewSpace, viewDirection);

    // Final color (material * lighting)
    vec3 lighting = directionalLighting + pointLighting;
    vec3 finalColor = fragColor * lighting;
    
    fragColorOut = vec4(finalColor, 1.0);

    if (ubo.drawModes.y == 1) {
        // Get Fresnel value
        float cosTheta = clamp(dot(normalViewSpace, viewDirection), 0.0, 1.0);
        float fresnel = fresnelFactor(cosTheta);
        
        // get reflection
        vec3 reflection = clampedReflect(-viewDirection, normalViewSpace);

        // get reflection on cornel box
        vec3 cornelReflection = getCornellBoxReflectionColor(fragPositionVS, reflection);
        
        // mix values
        vec3 mixedColor = mix(finalColor, cornelReflection, fresnel);

        // final color
        fragColorOut = vec4(mixedColor, 1.0);
    }

    // checks for the normal view or fresnel effect
    if (ubo.drawModes.x == 1) {
        fragColorOut = vec4(normalColor, 1.0);
    }

    if (ubo.drawModes.z == 1) {
        fragColorOut = vec4(uvCoordinates, 0, 1.0);
    }
}
