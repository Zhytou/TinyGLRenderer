#version 450

#include "common_normal.glsl"

layout(location = 0) in vec3 iFragNormal;
layout(location = 1) in vec3 iFragTangent;
layout(location = 2) in vec2 iFragUV;

layout(binding = 1) uniform sampler2D tNormalMap;

layout(location = 0) out vec4 oFragNormal;

void main() {
    oFragNormal = vec4(N_encode(N_toWorld(
        iFragNormal, 
        iFragTangent, 
        N_decode(texture(tNormalMap, iFragUV).xyz)
    )), 0.0);
}