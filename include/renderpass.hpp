#pragma once
#include <string>
#include <vector>

#include "framebuffer.hpp"

namespace tinyglrenderer {

enum class LoadOp {
    LOAD_OP_LOAD,
    LOAD_OP_CLEAR,
    LOAD_OP_DONT_CARE
};

enum class StoreOp {
    STORE_OP_STORE,
    STORE_OP_DONT_CARE
};

struct ClearValue {
    glm::vec4 color                    = {0.0f, 0.0f, 0.0f, 1.0f};
    std::pair<float, int> depthStencil = {1.0f, 0};
};

struct AttachmentDesc {
    std::string name;
    GLenum target     = GL_NONE;  // draw target (e.g., GL_COLOR/GL_DEPTH/GL_STENCIL/GL_DEPTH_STENCIL)
    GLenum type       = GL_NONE;  // storage type, could be texture/renderbuffer for off-screen rendering, or none for on-screen rendering (e.g., GL_TEXTURE_2D/GL_TEXTURE_CUBE_MAP/GL_RENDERBUFFER/GL_NONE)
    GLenum format     = GL_NONE;  // internal format (e.g., GL_RGBA8/GL_RGB3/GL_DEPTH_COMPONENT24/GL_STENCIL_INDEX8/GL_NONE)
    GLenum slot       = GL_NONE;  // attachment slot for off-screen rendering and on-screen rendering(e.g., GL_COLOR_ATTACHMENT0~GL_COLOR_ATTACHMENT15/GL_FRONT/GL_BACK/GL_DEPTH_ATTACHMENT/GL_STENCIL_ATTACHMENT)
    GLsizei layers    = 1;        // number of layers for texture attachment for cube map
    GLsizei mipLevels = 1;        // number of mip levels for texture attachment
    LoadOp loadOp     = LoadOp::LOAD_OP_DONT_CARE;
    StoreOp storeOp   = StoreOp::STORE_OP_DONT_CARE;
    ClearValue value  = {
        .color        = {0.0f, 0.0f, 0.0f, 1.0f},
        .depthStencil = {1.0f, 0}
    };
};

/**
 * @brief Execution blueprint defining how attachments are managed across a rendering phase.
 * @note Decouples layout intent (Load/Store operations) from actual hardware memory.
 * Provides structural layout compatibility matching modern API pass architectures.
 */
struct RenderPass {
    std::vector<AttachmentDesc> attachments;

    void begin(const std::shared_ptr<FrameBuffer>& framebuffer) {
        framebuffer->bind();
        for (auto attachment : attachments) {
            if (attachment.loadOp != LoadOp::LOAD_OP_CLEAR) {
                continue;
            }
            if (attachment.target == GL_COLOR) {
                framebuffer->clear(attachment.slot, attachment.value.color);
            } else {
                framebuffer->clear(attachment.target, attachment.value.depthStencil.first, attachment.value.depthStencil.second);
            }
        }
    }
    void end() {
    }
};

};  // namespace tinyglrenderer