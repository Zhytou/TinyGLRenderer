#version 450

#include "common_brdf.glsl"
#include "common_normal.glsl"
#include "common_shadow.glsl"
#include "common_screenspace.glsl"

layout(location = 0) in vec2 iFragUV;

layout(binding = 8) uniform sampler2D tAlbedoMap;
layout(binding = 9) uniform sampler2D tNormalMap;
layout(binding = 10) uniform sampler2D tMRAOMap;
layout(binding = 11) uniform sampler2D tDepthMap; 

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

layout(location = 0) out vec4 oFragColor;

void main() {
    float roughness = texture(tMRAOMap, iFragUV).y;
    float depth = texture(tDepthMap, iFragUV).x;

    vec3 N = N_decode(texture(tNormalMap, iFragUV).xyz);

    vec3 worldPos = Pos_toWorld(iFragUV, depth, uViewMatrix, uProjMatrix);
    vec3 direction = GGXSample(iFragUV, N, roughness);

    vec4 hit;
    // = RayMarch(worldPos, direction, uInvViewMatrix, uInvProjMatrix, tDepthMap, near, far, camera, face);

    if (hit.w == 0.0) {
        oFragColor = vec4(0.0);
        return;
    }

    oFragColor = vec4(1.0);
}
