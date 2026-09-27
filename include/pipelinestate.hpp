// RenderState.h
#pragma once
#include <glad/glad.h>

#include <memory>

#include "shader.hpp"

namespace tinyglrenderer {

/**
 * @brief Data block containing the fixed-function states of the graphics pipeline.
 * @note Encapsulates discrete OpenGL states (rasterization, blending, depth/stencil)
 * into a single data block. This data-driven approach prevents state leakage
 * and mimics modern Pipeline State Objects (PSO) found in Vulkan and DX12.
 */
struct PipelineState {
    // viewport
    GLboolean viewportDynamic = GL_FALSE;
    GLint viewX               = 0;
    GLint viewY               = 0;
    GLsizei viewW             = 1;
    GLsizei viewH             = 1;

    // rasterization config
    GLenum polygonMode   = GL_FILL;
    GLboolean cullEnable = GL_FALSE;
    GLenum cullMode      = GL_BACK;
    GLenum frontFace     = GL_CCW;

    // color blend config
    GLboolean blendEnable = GL_FALSE;
    GLenum srcBlend       = GL_SRC_ALPHA;
    GLenum dstBlend       = GL_ONE_MINUS_SRC_ALPHA;

    // depth test config
    GLboolean depthTestEnable  = GL_TRUE;
    GLboolean depthWriteEnable = GL_TRUE;
    GLenum depthFunc           = GL_LESS;

    // stencil test config
    GLboolean stencilTestEnable = GL_FALSE;
    GLenum stencilFunc          = GL_ALWAYS;
    GLint stencilRef            = 0;
    GLuint stencilMask          = 0xFFFFFFFF;
    GLuint stencilWriteMask     = 0xFFFFFFFF;

    // scissor test config
    GLboolean scissorTestEnable = GL_FALSE;
    GLint scissorX              = 0;
    GLint scissorY              = 0;
    GLsizei scissorW            = 1;
    GLsizei scissorH            = 1;

    inline void apply();
    inline void view(GLint x, GLint y, GLsizei w, GLsizei h) {
        if (viewportDynamic) {
            glViewport(x, y, w, h);
        }
    }
};

inline void PipelineState::apply() {
    glViewport(viewX, viewY, viewW, viewH);

    if (cullEnable) {
        glEnable(GL_CULL_FACE);
        glCullFace(cullMode);
    } else {
        glDisable(GL_CULL_FACE);
    }
    glPolygonMode(GL_FRONT_AND_BACK, polygonMode);
    glFrontFace(frontFace);

    if (blendEnable) {
        glEnable(GL_BLEND);
        glBlendFunc(srcBlend, dstBlend);
    } else {
        glDisable(GL_BLEND);
    }

    if (depthTestEnable) {
        glEnable(GL_DEPTH_TEST);
        glDepthFunc(depthFunc);
    } else {
        glDisable(GL_DEPTH_TEST);
    }
    glDepthMask(depthWriteEnable);

    if (stencilTestEnable) {
        glEnable(GL_STENCIL_TEST);
        glStencilFunc(stencilFunc, stencilRef, stencilMask);
        glStencilMask(stencilWriteMask);
    } else {
        glDisable(GL_STENCIL_TEST);
    }

    if (scissorTestEnable) {
        glEnable(GL_SCISSOR_TEST);
        glScissor(scissorX, scissorY, scissorW, scissorH);
    } else {
        glDisable(GL_SCISSOR_TEST);
    }
}

}  // namespace tinyglrenderer