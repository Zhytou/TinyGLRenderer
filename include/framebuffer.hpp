#pragma once
#include <glad/glad.h>

#include <glm/glm.hpp>
#include <unordered_map>

#include "texture.hpp"
#include "utils.hpp"

namespace tinyglrenderer {

/**
 * @brief Explicit physical render target allocation block.
 * @note Serves as the actual VRAM instance containing color, depth, or stencil surfaces.
 * Managed as a concrete asset bucket mapping directly to standard or off-screen render layouts.
 */
class FrameBuffer {
   public:
    FrameBuffer(bool screen, GLsizei w, GLsizei h);
    ~FrameBuffer();
    FrameBuffer(const FrameBuffer&)            = delete;
    FrameBuffer& operator=(const FrameBuffer&) = delete;
    FrameBuffer(FrameBuffer&& other);
    FrameBuffer& operator=(FrameBuffer&& other);

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    void setWidth(GLsizei w) { m_width = w; }
    void setHeight(GLsizei h) { m_height = h; }

    // Validate the framebuffer completeness.
    // @return True if the framebuffer is complete, False otherwise.
    // @note Default framebuffer is always complete.
    bool validate() const;
    // Bind the framebuffer to the current render target.
    void bind() const;
    // Attach a texture to the framebuffer.
    // @param slot The attachment slot to bind the texture to.
    // @param texture The attachment(namely texture) to bind.
    // @param level The mip level to bind.
    void attach(GLenum slot, const std::shared_ptr<Texture>& texture, GLint level = 0, GLint layer = 0);
    // Finalize the drawable attachments settings of the framebuffer.
    void finalize();
    // Clear the color attachment.
    // @note Must be called after bind().
    // @param slot The color attachment slot to clear(GL_COLOR_ATTACHMENT0/GL_COLOR_ATTACHMENT8).
    // @param color The clear color.
    // @param depth The clear depth.
    // @param stencil The clear stencil value.
    void clear(GLenum slot, const glm::vec4& color);
    // Clear the depth and stencil attachment.
    // @note Must be called after bind().
    // @param target The attachment target to clear(GL_DEPTH/GL_STENCIL/GL_DEPTH_STENCIL).
    // @param depth The clear depth.
    // @param stencil The clear stencil value.
    void clear(GLenum target, float depth, int stencil);
    // Copy the framebuffer content to the default framebuffer for presentation.
    // @note For GL_COLOR_BUFFER_BIT copy, the attachment slot must be set explicitly to GL_COLOR_ATTACHMENT0/GL_COLOR_ATTACHMENT31 for non-screen framebuffer and GL_BACK/GL_FRONT for screen framebuffer. Also, glFramebufferReadbuffer will be called in copy() for source attachment.
    // @param other The source framebuffer to copy from.
    // @param mask The bitfield of buffers to copy (e.g., GL_COLOR_BUFFER_BIT/GL_DEPTH_BUFFER_BIT/GL_STENCIL_BUFFER_BIT).
    // @param filter The interpolation method to apply if the image is stretched (e.g., GL_NEAREST/GL_LINEAR).
    // @param srcAttachment The source attachment slot to copy from.
    // @param dstAttachment The destination attachment slot to copy to.
    void copy(const FrameBuffer& other, GLenum mask, GLenum filter = GL_NEAREST, GLenum srcAttachment = GL_COLOR_ATTACHMENT0, GLenum dstAttachment = GL_COLOR_ATTACHMENT0);
    // Read the framebuffer attachment.
    // @param target The attachment target to read(GL_COLOR/GL_DEPTH/GL_STENCIL/GL_DEPTH_STENCIL).
    // @param slot The attachment slot to read.
    // @param data The vector to store the read data.
    // @param format The format of the read data(GL_RGB/GL_RED/GL_DEPTH_COMPONENT).
    template <typename T> void read(std::vector<T>& data, GLenum target, GLenum slot, GLenum format);
    // Divides the total framebuffer canvas into a compact, asymmetric grid of sub-tiles.
    // @param[out] rects Contains viewport rectangles parsed as 4-element groups [X, Y, Width, Height] in absolute pixel coordinates, ready for direct engine glViewport injection.
    // @param[out] remaps Contains texture coordinate remapping scales and offsets parsed as 4-element groups [offsetX, offsetY, scaleX, scaleY].
    // @param[in]  count The total number of active elements (e.g., shadow-casting light sources) to accommodate.
    void divide(std::vector<int>& rects, std::vector<float>& remaps, size_t count, int tileX = 1000, int tileY = 1000, int padding = 12);

   private:
    GLuint m_id      = 0;
    GLsizei m_width  = 0;
    GLsizei m_height = 0;
    std::unordered_map<GLenum, std::shared_ptr<Texture>> m_attachments;
};

template <typename T> void FrameBuffer::read(std::vector<T>& data, GLenum target, GLenum slot, GLenum format) {
    if (m_id != 0 && m_attachments.count(slot) == 0) { return; }

    if (target == GL_COLOR) {
        if (m_id == 0) {
            glReadBuffer(slot);
        } else {
            glNamedFramebufferReadBuffer(m_id, slot);
        }
    }

    if constexpr (std::is_same_v<T, float>) {
        glReadPixels(0, 0, m_width, m_height, format, GL_FLOAT, data.data());
    } else if constexpr (std::is_same_v<T, uint16_t>) {
        glReadPixels(0, 0, m_width, m_height, format, GL_UNSIGNED_SHORT, data.data());
    } else { // std::is_same_v<T, uint8_t>
        glReadPixels(0, 0, m_width, m_height, format, GL_UNSIGNED_BYTE, data.data());
    }
}

} // namespace tinyglrenderer
