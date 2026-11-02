#version 450

layout(location = 0) in vec2 iVertPos;
layout(location = 1) in vec2 iVertUV;

layout(location = 0) out vec2 oFragUV;

void main() {
    oFragUV = iVertUV;

    gl_Position = vec4(iVertPos, 0.0, 1.0);
}