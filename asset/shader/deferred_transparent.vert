#version 450

#include "common_normal.glsl"

layout(location = 0) in vec3 iVertPos;
layout(location = 1) in vec3 iVertNormal;
layout(location = 2) in vec3 iVertTangent;
layout(location = 3) in vec2 iVertUV;

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
layout(std140, binding = 1) uniform ModelBlock {
    mat4 uModelMatrix;
    mat4 uNormalMatrix;
};

layout(location = 0) out vec3 oFragNormal;
layout(location = 1) out vec3 oFragTangent;
layout(location = 2) out vec2 oFragUV;

void main() {
    oFragNormal = (uNormalMatrix * vec4(iVertNormal, 0.0)).xyz;
    oFragTangent = (uModelMatrix * vec4(iVertTangent, 0.0)).xyz;
    oFragUV = iVertUV;

    gl_Position = uProjMatrix * uViewMatrix * uModelMatrix * vec4(iVertPos, 1.0);
}