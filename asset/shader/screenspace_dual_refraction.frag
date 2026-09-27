#version 450

#include "common_brdf.glsl"
#include "common_normal.glsl"
#include "common_shadow.glsl"
#include "common_sampling.glsl"
#include "common_screenspace.glsl"

layout(location = 0) in vec2 iFragUV;

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

layout(binding = 6) uniform sampler2D tBackNormalMap;
layout(binding = 7) uniform sampler2D tBackDepthMap;
layout(binding = 8) uniform sampler2D tFrontAlbedoMap;
layout(binding = 9) uniform sampler2D tFrontNormalMap; 
layout(binding = 10) uniform sampler2D tFrontMAROMap; 
layout(binding = 11) uniform sampler2D tFrontDepthMap;
layout(binding = 12) uniform samplerCube tSkyboxMap;
layout(binding = 20) uniform sampler2D tScreenColorMap;
layout(binding = 23) uniform sampler2D tScreenDepthMap;
layout(binding = 24) uniform sampler2D tScreenTransparentColorMap;

out vec4 oFragColor;

void main() {
    // ----------------------------------------------------------------
    // Evaluate refraction color
    // https://cwyman.org/papers/sig05_approxISRefr.pdf
    // ----------------------------------------------------------------
    vec3 refractionColor = vec3(0.0);
    const float epsilon = 0.001;
    const float ior = 1.5;
    vec2 UV1 = iFragUV;
    float D1 = texture(tFrontDepthMap, UV1).x;
    float D2 = texture(tBackDepthMap, UV1).x;
    float DD1 = Depth_toLinear(D1, uNear, uFar);
    float DD2 = Depth_toLinear(D2, uNear, uFar);

    vec3 P1 = Pos_toWorld(UV1, D1, uInvViewMatrix, uInvProjMatrix);
    vec3 N1 = N_decode(texture(tFrontNormalMap, UV1).xyz);
    vec3 V = normalize(uCameraPos - P1);
    vec3 T1 = normalize(refract(-V, N1, 1.0 / ior)); 

    vec3 P2 = P1 + T1 * (DD2 - DD1);
    vec2 UV2 = Pos_toScreen(P2, uViewMatrix, uProjMatrix).xy;
    vec3 N2 = N_decode(texture(tBackNormalMap, UV2).xyz);
    if (dot(N2, N2) < epsilon) {
        N2 = normalize(T1 + V * dot(T1, -V));        
    } else {
        N2 = normalize(N2);
    }

    vec3 T2 = refract(T1, -N2, ior);
    if (dot(T2, T2) < epsilon) { // TIR
        T2 = reflect(T1, -N2);
        T2 = T2 / -T2.z;
    }

    vec4 hit = RayMarch(P2, normalize(T2), uViewMatrix, uProjMatrix, tScreenDepthMap, uNear, uFar, 0.0, 0.0);
    if (hit.w > 0.0) {
        refractionColor = texture(tScreenColorMap, hit.xy).rgb;
    } else {
        refractionColor = texture(tSkyboxMap, normalize(T2)).rgb;
    }

    // ----------------------------------------------------------------
    // Evaluate reflection color
    // ----------------------------------------------------------------
    vec3 reflectionColor = texture(tScreenTransparentColorMap, UV1).rgb;

    // ----------------------------------------------------------------
    // Evaluate final color
    // ----------------------------------------------------------------
    vec3 N = normalize(N1);
    vec3 F0 = mix(vec3(0.04), texture(tFrontAlbedoMap, UV1).rgb, texture(tFrontMAROMap, UV1).x);
    vec3 F = F_Schlick(dot(N, V), F0);
    oFragColor = vec4(mix(refractionColor, reflectionColor, F), 1.0);
}

