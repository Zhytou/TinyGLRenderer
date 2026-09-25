#include "framebuffer.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

#include "utils.hpp"

namespace tinyglrenderer {

FrameBuffer::FrameBuffer(bool screen, GLsizei w, GLsizei h)
    : m_width(w),
      m_height(h) {
    if (!screen) {
        // Create framebuffer object
        //
        // ┌──────────────────────────────────────────────────────────────────────────────────────────────┐
        // │                        glGenFramebuffers vs glCreateFramebuffers                              │
        // ├───────────────────────────────────────────────┬──────────────────────────────────────────────┤
        // │              glGenFramebuffers                │              glCreateFramebuffers             │
        // ├───────────────────────────────────────────────┼──────────────────────────────────────────────┤
        // │ • Generates UNINITIALIZED IDs                 │ • Creates and INITIALIZES objects            │
        // │ • Requires separate glBindFramebuffer call    │ • No bind needed (DSA style)                  │
        // │ • Changes global binding state                │ • Direct state access, no side effects       │
        // │ • Must call glBindFramebuffer before config   │ • Configure directly via glNamed... APIs     │
        // │ • Traditional API (OpenGL 3.0+)               │ • Modern API (OpenGL 4.5+ / ARB_dsa)          │
        // │ • More verbose (gen + bind + config)          │ • Concise (create + direct config)           │
        // │ • Error prone (forget to bind)                │ • Safer (no binding state to track)          │
        // └───────────────────────────────────────────────┴──────────────────────────────────────────────┘
        //
        // Key insight:  glGenFramebuffers  = "allocate ID only" (need bind to use)
        //               glCreateFramebuffers = "allocate + initialize" (ready to use immediately)
        glCreateFramebuffers(1, &m_id);
    }
}

FrameBuffer::~FrameBuffer() {
    if (m_id) { glDeleteFramebuffers(1, &m_id); }
    m_attachments.clear();
}

FrameBuffer::FrameBuffer(FrameBuffer&& other) {
    if (m_id) {
        glDeleteFramebuffers(1, &m_id);
        m_attachments.clear();
    }

    m_id          = other.m_id;
    m_width       = other.m_width;
    m_height      = other.m_height;
    m_attachments = std::move(other.m_attachments);

    other.m_id     = 0;
    other.m_width  = 0;
    other.m_height = 0;
    other.m_attachments.clear();
}

FrameBuffer& FrameBuffer::operator=(FrameBuffer&& other) {
    if (m_id) {
        glDeleteFramebuffers(1, &m_id);
        m_attachments.clear();
    }

    m_id          = other.m_id;
    m_width       = other.m_width;
    m_height      = other.m_height;
    m_attachments = std::move(other.m_attachments);

    other.m_id     = 0;
    other.m_width  = 0;
    other.m_height = 0;
    other.m_attachments.clear();
    return *this;
};

bool FrameBuffer::validate() const {
    // Default framebuffer is always complete.
    GLenum status = glCheckNamedFramebufferStatus(m_id, GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) { std::cerr << "FrameBuffer[" << m_id << "]::validate: framebuffer not complete" << std::endl; }
    return status == GL_FRAMEBUFFER_COMPLETE;
}

void FrameBuffer::bind() const { glBindFramebuffer(GL_FRAMEBUFFER, m_id); }

void FrameBuffer::attach(GLenum slot, const std::shared_ptr<Texture>& texture, GLint level, GLint layer) {
    if (m_id == 0) { throw std::runtime_error("FrameBuffer::attach: framebuffer not created"); }
    m_attachments[slot] = texture;

    if (texture->getTarget() == GL_TEXTURE_2D) {
        // Attach a 2D texture level to a framebuffer
        glNamedFramebufferTexture(m_id, slot, texture->getID(), level);
    } else if (texture->getTarget() == GL_TEXTURE_CUBE_MAP || texture->getTarget() == GL_TEXTURE_2D_ARRAY || texture->getTarget() == GL_TEXTURE_3D) {
        // Attach a cube map texture level to a framebuffer
        glNamedFramebufferTextureLayer(m_id, slot, texture->getID(), level, layer);
    } else {
        throw std::runtime_error("FrameBuffer::attach: attachment target not supported");
    }
    
}

void FrameBuffer::finalize() {
    if (m_id == 0) { return; }

    // Get the attached color slots
    std::vector<GLenum> slots;
    for (auto [slot, _] : m_attachments) {
        if (slot >= GL_COLOR_ATTACHMENT0 && slot <= GL_COLOR_ATTACHMENT31) { slots.push_back(slot); }
    }

    if (slots.empty()) {
        slots = { GL_NONE };
    } else {
        // !Warning: Attachment order in glDrawBuffers/glNamedFramebufferDrawBuffers must match fragment shader's layout(location = N) order.
        std::sort(slots.begin(), slots.end());
    }

    // Activate the attached color textures as drawable attachments
    // Depth and stencil attachments are activated by DEPTH_WRITE_MASK and STENCIL_WRITE_MASK.
    // ┌──────────────────────────────────────────────────────────────────────────────────────────────┐
    // │                    glDrawBuffers vs glNamedFramebufferDrawBuffers                           │
    // ├───────────────────────────────────────────────┬──────────────────────────────────────────────┤
    // │              glDrawBuffers                    │         glNamedFramebufferDrawBuffers       │
    // ├───────────────────────────────────────────────┼──────────────────────────────────────────────┤
    // │ • Requires currently BOUND framebuffer        │ • Directly specifies framebuffer ID         │
    // │ • Must call glBindFramebuffer first           │ • No bind needed (DSA style)                │
    // │ • Changes global framebuffer state            │ • Stateless, no side effects                │
    // │ • Traditional API (OpenGL 3.0+)               │ • Modern API (OpenGL 4.5+ / ARB_dsa)        │
    // └───────────────────────────────────────────────┴──────────────────────────────────────────────┘
    //
    // Key insight:  glDrawBuffers                    = "bind then configure" (state-dependent)
    //               glNamedFramebufferDrawBuffers    = "configure directly" (stateless, explicit)
    glNamedFramebufferDrawBuffers(m_id, slots.size(), slots.data());
}

void FrameBuffer::clear(GLenum slot, const glm::vec4& color) {
    int bufferIndex = -1;
    if (m_id != 0 && slot >= GL_COLOR_ATTACHMENT0 && slot <= GL_COLOR_ATTACHMENT15 && m_attachments.count(slot)) {
        bufferIndex = slot - GL_COLOR_ATTACHMENT0;
    } else if (m_id == 0 && slot == GL_BACK) {
        bufferIndex = 0;
    }

    if (bufferIndex >= 0) {
        // Clear color attachment
        //
        // ┌──────────────────────────────────────────────────────────────────────────────────────────────┐
        // │                     glClearBufferfv vs glClearNamedFramebufferfv                            │
        // ├───────────────────────────────────────────────┬──────────────────────────────────────────────┤
        // │              glClearBufferfv                  │              glClearNamedFramebufferfv       │
        // ├───────────────────────────────────────────────┼──────────────────────────────────────────────┤
        // │ • Clears currently BOUND framebuffer          │ • Directly specifies framebuffer ID          │
        // │ • Requires glBindFramebuffer first            │ • No bind required (DSA style)                │
        // │ • Traditional API (OpenGL 3.0+)               │ • Modern API (OpenGL 4.5+ / ARB_dsa)         │
        // │ • State-dependent, error prone                │ • Explicit, stateless, safer                 │
        // └───────────────────────────────────────────────┴──────────────────────────────────────────────┘
        //
        // Key insight:  glClearBufferfv        = "bind then clear" (state-dependent)
        //               glClearNamedFramebufferfv = "clear directly" (stateless, explicit)
        //
        // Suffix meanings: fv = float vector → clears COLOR (RGBA) or DEPTH (single float or float vector)
        //                  fi = float + int  → clears DEPTH_STENCIL (depth + stencil together)
        //                  iv = int vector → clears STENCIL (single int or int vector)
        if (m_id == 0) {
            glClearBufferfv(GL_COLOR, bufferIndex, &color.x);
        } else {
            glClearNamedFramebufferfv(m_id, GL_COLOR, bufferIndex, &color.x);
        }
    }
}

void FrameBuffer::clear(GLenum target, float depth, int stencil) {
    if (m_id == 0) {
        if (target == GL_DEPTH) {
            glClearBufferfv(target, 0, &depth);
        } else if (target == GL_STENCIL) {
            glClearBufferiv(target, 0, &stencil);
        } else if (target == GL_DEPTH_STENCIL) {
            glClearBufferfi(target, 0, depth, stencil);
        }
    } else {
        if (target == GL_DEPTH && m_attachments.count(GL_DEPTH_ATTACHMENT)) {
            glClearNamedFramebufferfv(m_id, target, 0, &depth);
        } else if (target == GL_STENCIL && m_attachments.count(GL_STENCIL_ATTACHMENT)) {
            glClearNamedFramebufferiv(m_id, target, 0, &stencil);
        } else if (target == GL_DEPTH_STENCIL && m_attachments.count(GL_DEPTH_STENCIL_ATTACHMENT)) {
            glClearNamedFramebufferfi(m_id, target, 0, depth, stencil);
        }
    }
}

void FrameBuffer::copy(const FrameBuffer& other, GLenum mask, GLenum filter, GLenum srcAttachment, GLenum dstAttachment) {
    if (mask & GL_COLOR_BUFFER_BIT) {
        glNamedFramebufferReadBuffer(other.m_id, srcAttachment);
        glNamedFramebufferDrawBuffer(m_id, dstAttachment);
    }

    // Blit framebuffer region
    //
    // ┌──────────────────────────────────────────────────────────────────────────────────────────────┐
    // │                     glBlitFramebuffer vs glBlitNamedFramebuffer                               │
    // ├───────────────────────────────────────────────┬──────────────────────────────────────────────┤
    // │              glBlitFramebuffer                │              glBlitNamedFramebuffer           │
    // ├───────────────────────────────────────────────┼──────────────────────────────────────────────┤
    // │ • Require glBindFramebuffer to set            │ • Directly pass FBO ID as parameter           │
    // │   GL_READ_FRAMEBUFFER / GL_DRAW_FRAMEBUFFER    │ • No bind operation required (DSA style)      │
    // │ • Traditional API (OpenGL 3.0+)               │ • Modern API (OpenGL 4.5+ / ARB_dsa)         │
    // │ • State-dependent, easy to mix up bind target  │ • Explicit & stateless, safer to use         │
    // │ • read/draw target can be 0 (default FBO)      │ • readFramebuffer/drawFramebuffer CAN BE 0   │
    // │   when bound                                  │ • 0 represents default screen framebuffer    │
    // └───────────────────────────────────────────────┴──────────────────────────────────────────────┘
    //
    // Key insight:  glBlitFramebuffer        = "bind read/draw target first then blit" (state-dependent)
    //               glBlitNamedFramebuffer   = "specify read/draw FBO ID directly" (stateless, explicit)
    // Note: Both APIs support passing 0 for framebuffer ID, 0 means the default screen framebuffer.
    //       Depth/stencil blit only accepts GL_NEAREST filter mode, GL_LINEAR is invalid.
    glBlitNamedFramebuffer(other.m_id, // srcFramebuffer
        m_id,                          // dstFramebuffer
        0,
        0, // srcX0, srcY0
        other.m_width,
        other.m_height, // srcX1, srcY1
        0,
        0, // dstX0, dstY0
        m_width,
        m_height, // dstX1, dstY1
        mask,
        filter);
}

void FrameBuffer::divide(std::vector<int>& rects, std::vector<float>& remaps, size_t count, int tileX, int tileY, int padding) {
    if (count == 0) { return; }

    int n     = static_cast<int>(std::ceil(std::sqrt(count)));              // number of grid columns
    int m     = static_cast<int>(std::ceil(static_cast<float>(count) / n)); // number of grid rows
    int cellX = tileX + padding;
    int cellY = tileY + padding;
    if (m_width < cellX * n || m_height < cellY * m) { return; }

    rects.clear();
    remaps.clear();
    rects.reserve(count * 4);  // [viewportX, viewportY, viewportW, viewportH]
    remaps.reserve(count * 4); // [offsetX, offsetY, scaleX, scaleY]

    size_t lightIdx = 0;
    for (int i = 0; i < m; i++) {
        for (int j = 0; j < n; j++) {
            if (lightIdx >= count)
                break;

            int x = j * cellX;
            int y = i * cellY;
            rects.push_back(x);
            rects.push_back(y);
            rects.push_back(tileX);
            rects.push_back(tileY);

            float offsetX = static_cast<float>(x) / (m_width * 1.0f);
            float offsetY = static_cast<float>(y) / (m_height * 1.0f);
            float scaleX  = static_cast<float>(tileX) / (m_width * 1.0f);
            float scaleY  = static_cast<float>(tileY) / (m_height * 1.0f);
            remaps.push_back(offsetX);
            remaps.push_back(offsetY);
            remaps.push_back(scaleX);
            remaps.push_back(scaleY);

            lightIdx++;
        }
    }
}

} // namespace tinyglrenderer
