#version 450

#include "common_brdf.glsl"
#include "common_normal.glsl"
#include "common_shadow.glsl"
#include "common_sampling.glsl"
#include "common_raymarch.glsl"

layout(location = 0) in vec3 iFragPos;
layout(location = 1) in vec3 iFragNormal;
layout(location = 2) in vec3 iFragTangent;
layout(location = 3) in vec2 iFragUV;
layout(location = 4) in vec3 iFragView; // view direction from vertex to camera
layout(location = 5) in vec2 iFragScreenUV;
layout(location = 6) in vec3 iFragCenter;

// ubo block
layout(std140, binding = 0) uniform CameraBlock {
    mat4 uViewMatrix;
    mat4 uProjMatrix;
    mat4 uInvViewMatrix;
    mat4 uInvProjMatrix;
    vec3 uCameraPos;
    float uCameraType;
    float uFov;
    float uNear;
    float uFar;
    float uAspect;
};
// ssbo array
struct Light {
    mat4 viewProjMatrix; 
    vec4 colorIntensity;
    vec4 vectorType; // use .w to distinguish between directional and point light
    vec4 uvOffsetScale;
};
layout(std430, binding = 0) buffer LightBuffer {
    Light uLights[];
};
uniform int uLightCount;
uniform float uDistortion = 1.0;

layout(binding = 0) uniform sampler2D tAlbedoMap;
layout(binding = 1) uniform sampler2D tNormalMap;
layout(binding = 2) uniform sampler2D tMRAOMap;
layout(binding = 8) uniform sampler2D tBackNormalMap; // backface normal and depth of transparent object, important for refraction bouncing
layout(binding = 9) uniform sampler2D tBackDepthMap;
layout(binding = 10) uniform sampler2D tFrontNormalMap; // frontface normal and depth of transparent object, important for refraction bouncing
layout(binding = 11) uniform sampler2D tFrontDepthMap;
layout(binding = 12) uniform samplerCube tSkyboxMap;
layout(binding = 14) uniform samplerCube tIBLDiffuseMap;
layout(binding = 15) uniform samplerCube tIBLSpecularMap;
layout(binding = 16) uniform sampler2D tIBLBRDFLUTMap;
layout(binding = 19) uniform sampler2D tShadowDepthMap; // GL_DEPTH_COMPONENT24, .x is the depth value;
layout(binding = 20) uniform sampler2D tScreenColorMap;
layout(binding = 23) uniform sampler2D tScreenDepthMap;

out vec4 oFragColor;

void main() {
    vec3 P = iFragPos;
    vec2 UV = iFragUV;
    vec2 screenUV = iFragScreenUV;

    vec3 albedo = texture(tAlbedoMap, UV).rgb;
    vec3 mrao   = texture(tMRAOMap, UV).rgb;
    float metallic  = mrao.r;
    float roughness = mrao.g;
    float ao        = mrao.b;

    vec3 F0 = mix(vec3(0.04), albedo, metallic);
    vec3 V = normalize(iFragView); // frag -> camera
    vec3 N = normalize(iFragNormal); // primitive normal in world space
    vec3 T = normalize(iFragTangent);
    vec3 TN = N_decode(texture(tNormalMap, UV).xyz); // frag normal in tangent space
    N = N_toWorld(N, T, TN);  // convert frag normal to world space with help of primitive normal and frag tangent
    float NdotV = clamp(dot(N, V), 0.0, 1.0);

    // ----------------------------------------------------------------
    // Evaluate direct light reflection color(both diffuse and specular)
    // ----------------------------------------------------------------
    vec3 dReflectionColor = vec3(0.0);
    for (int i = 0; i < uLightCount; i++) {
        vec3 L = uLights[i].vectorType.w == 0.0 ? normalize(-uLights[i].vectorType.xyz) : normalize(uLights[i].vectorType.xyz - P); // frag -> light
        vec3 lightSpaceUVD = Pos_toLightSpaceUVD(uLights[i].viewProjMatrix, P);
        vec2 atlasUV = uLights[i].uvOffsetScale.xy + lightSpaceUVD.xy * uLights[i].uvOffsetScale.zw;

        vec3 color = BRDF(L, V, N, F0, albedo, metallic, roughness) * uLights[i].colorIntensity.rgb * uLights[i].colorIntensity.w;
        float visibility = SM(tShadowDepthMap, atlasUV, lightSpaceUVD.z);
        
        dReflectionColor += color * visibility;
    } 

    // ----------------------------------------------------------------
    // Evaluate indirect light reflection color
    // ----------------------------------------------------------------
    vec3 indReflectionColor = vec3(0.0);

    // ----------------------------------------------------------------
    // Evaluate both direct and indirect light refraction color
    // ----------------------------------------------------------------
    vec3 refractionColor = vec3(0.0);
    const float epsilon = 0.03;
    const int bounce = 5;
    const float ior = 2.2;
    float eta = 1.0 / ior; // air to glass
    vec3 H = normalize(GGXSample(UV, N, roughness));
    vec3 R = normalize(refract(-V, H, eta)); 
    vec3 absorption = vec3(0.1, 0.02, 0.15);

    for (int i = 0; i < bounce; ++i) {
        // 1. Raymarching to find the intersection point
        vec4 hit;
        if (dot(V, R) > 0.0) {
            hit = RayMarch(P, R, uViewMatrix, uProjMatrix, tFrontDepthMap, uNear, uFar, uCameraType, 1.0);
            // N = N_decode(texture(tFrontNormalMap, hit.xy).xyz); 
        } else {
            hit = RayMarch(P, R, uViewMatrix, uProjMatrix, tBackDepthMap, uNear, uFar, uCameraType, 0.0);
            // N = N_decode(texture(tBackNormalMap, hit.xy).xyz); 
        }
        if (hit.w != 1.0) {
            if (dot(V, R) > 0.0) {
                refractionColor = texture(tSkyboxMap, R).rgb;
            }
            break;
        }

        // 2. Update position and normal at new hit point
        P = P + R * hit.z;
        N = normalize(P - iFragCenter);
        float NdotR = dot(N, R);
        if (NdotR > 0.0) { N = -N; } // ensure the normal is facing the ray direction

        // 3. Update the eta accordingly(TIR or not)
        eta = ior; // always try to get out of glass
        float k = 1.0 - eta * eta * (1.0 - NdotR * NdotR); // total internal reflection indicator (k < 0.0 -> TIR)

        // 4. Update the refraction ray direction and position accordingly(TIR or not)
        R = k <= 0.0 ? normalize(reflect(R, N)) : normalize(refract(R, N, eta));
        P = P + R * epsilon;

        // 5. Evaluate the final color
        if (k > 0.0) {
            hit = RayMarch(P, R, uViewMatrix, uProjMatrix, tScreenDepthMap, uNear, uFar, uCameraType, 0.0);
            
            if (hit.w == 1.0) {
                refractionColor = texture(tScreenColorMap, hit.xy).rgb;
            } else {
                // refractionColor = texture(tSkyboxMap, R).rgb;
            }
            break;
        } 
    
    }

    // ----------------------------------------------------------------
    // Evaluate final color
    // ----------------------------------------------------------------
    vec3 F = F_Schlick(NdotV, F0);
    oFragColor = vec4(mix(refractionColor, dReflectionColor + indReflectionColor, F), 1.0);
}

