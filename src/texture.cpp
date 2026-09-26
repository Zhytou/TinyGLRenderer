#include "texture.hpp"

#include <format>
#include <iostream>
#include <stdexcept>

#include "utils.hpp"

namespace tinyglrenderer {

namespace fs = std::filesystem;

Texture::Texture(GLsizei size, GLenum target, GLenum internalFormat, GLsizei mipLevels)
    : m_width(size),
      m_height(1),
      m_depth(1),
      m_target(target),
      m_internalFormat(internalFormat),
      m_mipLevels(mipLevels) {
    if (size == 0 || mipLevels > 10 || size < (1 << (mipLevels - 1))) { throw std::runtime_error(std::format("Texture::Texture: Invalid Texture size {} for mip level {}", size, mipLevels)); }

    glCreateTextures(m_target, 1, &m_id);

    if (m_target == GL_TEXTURE_1D) {
        glTextureStorage1D(m_id, m_mipLevels, m_internalFormat, size);
    } else {
        throw std::runtime_error("Texture::Texture: Texture target is not GL_TEXTURE_1D");
    }
}

Texture::Texture(GLsizei width, GLsizei height, GLenum target, GLenum internalFormat, GLsizei mipLevels)
    : m_width(width),
      m_height(height),
      m_depth(1),
      m_target(target),
      m_internalFormat(internalFormat),
      m_mipLevels(mipLevels) {
    if (width == 0 || height == 0 || mipLevels > 10 || width < (1 << (mipLevels - 1)) || height < (1 << (mipLevels - 1))) { throw std::runtime_error(std::format("Texture::Texture: Invalid Texture size {}x{} for mip level {}", width, height, mipLevels)); }

    // Create texture handle
    //
    // ┌──────────────────────────────────────────────────────────────────────────────────────────────┐
    // │                         glGenTextures vs glCreateTextures                                    │
    // ├───────────────────────────────────────────────┬──────────────────────────────────────────────┤
    // │              glGenTextures                    │              glCreateTextures                │
    // ├───────────────────────────────────────────────┼──────────────────────────────────────────────┤
    // │ • Generates UNINITIALIZED texture IDs         │ • Creates and INITIALIZES texture objects    │
    // │ • Requires separate glBindTexture call        │ • No bind needed (DSA style)                  │
    // │ • Must specify target at bind time            │ • Target specified at creation               │
    // │ • Changes global texture binding state        │ • Direct state access, no side effects       │
    // │ • Must call glBindTexture before config       │ • Configure directly via glTexture... APIs   │
    // │ • Traditional API (OpenGL 1.0+)               │ • Modern API (OpenGL 4.5+ / ARB_dsa)          │
    // │ • More verbose (gen + bind + config)          │ • Concise (create + direct config)           │
    // │ • Error prone (forget to bind)                │ • Safer (no binding state to track)          │
    // └───────────────────────────────────────────────┴──────────────────────────────────────────────┘
    //
    // Key insight:  glGenTextures  = "allocate ID only" (need bind + target to use)
    //               glCreateTextures = "allocate + initialize + target" (ready to configure)
    //
    // Example:
    //   Legacy: glGenTextures(1, &tex); glBindTexture(GL_TEXTURE_2D, tex); glTexImage2D(...);
    //   Modern: glCreateTextures(GL_TEXTURE_2D, 1, &tex); glTextureStorage2D(tex, ...);
    //
    glCreateTextures(m_target, 1, &m_id);

    // Allocate memory for texture storage
    //
    // ┌─────────────────────────────────────────────────────────────────────────────────┐
    // │                    glTexImage2D vs glTextureStorage2D                           │
    // ├──────────────────────────────┬──────────────────────────────────────────────────┤
    // │      glTexImage2D (Legacy)   │         glTextureStorage2D (Modern)              │
    // ├──────────────────────────────┼──────────────────────────────────────────────────┤
    // │ • Immutable AFTER upload     │ • COMPLETELY immutable (can't change format/size)│
    // │ • Can re-allocate anytime     │ • Must be allocated ONCE at creation             │
    // │ • Requires binding target     │ • Direct state access (no bind needed)           │
    // │ • Driver does more checking   │ • Driver can optimize better                    │
    // │ • Older API (< OpenGL 4.2)    │ • Modern API (OpenGL 4.2+ / DSA)                 │
    // └──────────────────────────────┴──────────────────────────────────────────────────┘
    //
    // Key insight: glTextureStorage2D = "allocate once, use forever" (best practice)
    //              glTexImage2D       = "flexible but slower" (legacy path)
    if (m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP || m_target == GL_TEXTURE_RECTANGLE || m_target == GL_TEXTURE_1D_ARRAY) {
        glTextureStorage2D(m_id, m_mipLevels, m_internalFormat, m_width, m_height);
    } else {
        throw std::runtime_error("Texture::Texture: Texture target is not GL_TEXTURE_2D or GL_TEXTURE_CUBE_MAP or GL_TEXTURE_RECTANGLE or GL_TEXTURE_1D_ARRAY");
    }
}

Texture::Texture(GLsizei width, GLsizei height, GLsizei depth, GLenum target, GLenum internalFormat, GLsizei mipLevels)
    : m_width(width),
      m_height(height),
      m_depth(depth),
      m_target(target),
      m_mipLevels(mipLevels),
      m_internalFormat(internalFormat) {
    if (width == 0 || height == 0 || depth == 0 || mipLevels > 10 || width < (1 << (mipLevels - 1)) || height < (1 << (mipLevels - 1))) { throw std::runtime_error(std::format("Texture::Texture: Invalid Texture size {}x{}x{} for mip level {}", width, height, depth, mipLevels)); }

    glCreateTextures(m_target, 1, &m_id);

    if (m_target == GL_TEXTURE_3D || m_target == GL_TEXTURE_2D_ARRAY || m_target == GL_TEXTURE_CUBE_MAP_ARRAY) {
        glTextureStorage3D(m_id, m_mipLevels, m_internalFormat, m_width, m_height, m_depth);
    } else {
        throw std::runtime_error("Texture::Texture: Texture target is not GL_TEXTURE_3D or GL_TEXTURE_2D_ARRAY or GL_TEXTURE_CUBE_MAP_ARRAY");
    }
}

Texture::Texture(Texture&& other) {
    if (m_id) { glDeleteTextures(1, &m_id); }
    m_id             = other.m_id;
    m_target         = other.m_target;
    m_width          = other.m_width;
    m_height         = other.m_height;
    m_depth          = other.m_depth;
    m_internalFormat = other.m_internalFormat;
    other.m_id       = 0;
}

Texture& Texture::operator=(Texture&& other) {
    if (m_id) { glDeleteTextures(1, &m_id); }
    m_id             = other.m_id;
    m_target         = other.m_target;
    m_width          = other.m_width;
    m_height         = other.m_height;
    m_depth          = other.m_depth;
    m_internalFormat = other.m_internalFormat;
    other.m_id       = 0;
    return *this;
}

Texture::~Texture() {
    if (m_id) { glDeleteTextures(1, &m_id); }
}

GLsizei Texture::getWidth(GLint level) const { return m_width / (1 << level); }

GLsizei Texture::getHeight(GLint level) const {
    if (m_target == GL_TEXTURE_1D) {
        return m_height; // 1
    }
    return m_height / (1 << level);
}

GLsizei Texture::getDepth(GLint level) const {
    if (m_target == GL_TEXTURE_1D || m_target == GL_TEXTURE_2D || m_target == GL_TEXTURE_CUBE_MAP || m_target == GL_TEXTURE_RECTANGLE || m_target == GL_TEXTURE_1D_ARRAY) {
        return m_depth; // 1
    }
    return m_depth / (1 << level);
}

void Texture::bind(GLuint slot) const {
    // Bind texture to a specific texture slot, namely the glsl binding index
    //
    // ┌─────────────────────────────────────────────────────────────────────────────────┐
    // │              glActiveTexture + glBindTexture  vs  glBindTextureUnit             │
    // ├───────────────────────────────────────────────┬─────────────────────────────────┤
    // │     Legacy: glActiveTexture + glBindTexture   │   Modern: glBindTextureUnit     │
    // ├───────────────────────────────────────────────┼─────────────────────────────────┤
    // │ • Two function calls                          │ • Single function call          │
    // │ • Changes global "active" state               │ • Direct, stateless binding     │
    // │ • Error-prone (forget glActiveTexture)        │ • Explicit slot specification   │
    // │ • Works with all OpenGL versions              │ • Requires OpenGL 4.5+ / DSA    │
    // │ • Must specify texture target (e.g., GL_TEXTURE_2D) │ • Target inferred from texture object │
    // │ • Slower (state validation overhead)          │ • Faster (direct state access)  │
    // └───────────────────────────────────────────────┴─────────────────────────────────┘
    glBindTextureUnit(slot, m_id);
}

void Texture::unbind(GLuint slot) const { glBindTextureUnit(slot, 0); }

void Texture::unbind() {
    static GLint slots = 0;
    if (slots == 0) { glGetIntegerv(GL_MAX_TEXTURE_UNITS, &slots); }
    for (GLint slot = 0; slot < slots; slot++) { glBindTextureUnit(slot, 0); }
}

void Texture::clear(const void* value, GLenum format, GLenum type, GLint level) {
    if (m_id == 0) { return; }
    GLsizei w = getWidth(level);
    GLsizei h = getHeight(level);
    GLsizei d = getDepth(level);
    glClearTexSubImage(m_id, level, 0, 0, 0, w, h, d, format, type, value);
}

void Texture::upload(const std::shared_ptr<Image>& img, GLint level) {
    // std::cout << "Uploading texture data from image [" << img->getWidth() << ' ' << img->getHeight() << ' ' << img->getChannels() << ' ' << std::showbase << std::hex << img->getFormat() << ' ' << img->getDataType() << std::dec << "] to level" << level << " GPU memory\n";

    // Check if image data is valid
    if (img == nullptr || img->getData() == nullptr) { throw std::runtime_error("Texture::upload: image data is null"); }
    if (level < 0 || level >= m_mipLevels) { throw std::runtime_error(std::format("Texture::upload: mip level {} out of range [0 - {}]", level, m_mipLevels)); }
    GLsizei width     = img->getWidth();
    GLsizei height    = img->getHeight();
    GLsizei texWidth  = getWidth(level);
    GLsizei texHeight = getHeight(level);
    if (width > texWidth || height > texHeight) { throw std::runtime_error(std::format("Texture::upload: image size {}x{} does not match texture size {}x{} at level {}", width, height, texWidth, texHeight, level)); }

    // Upload texture data to GPU memory
    //
    // ┌──────────────────────────────────────────────────────────────────────────────────────────┐
    // │                           glTexImage2D vs glTexSubImage2D                                 │
    // ├─────────────────────────────────────────────┬────────────────────────────────────────────┤
    // │              glTexImage2D                   │              glTexSubImage2D                │
    // ├─────────────────────────────────────────────┼────────────────────────────────────────────┤
    // │ • Allocates NEW texture storage             │ • Uploads data to EXISTING storage         │
    // │ • Can change format/size (re-allocate)      │ • Cannot change format/size                │
    // │ • Resets texture to uninitialized state     │ • Preserves other mip levels / regions     │
    // │ • Must specify full width/height            │ • Can update a sub-rectangle (x, y, w, h)  │
    // │ • More expensive (may reallocate GPU memory)│ • Cheap (just data transfer)               │
    // │ • Used for INITIAL allocation               │ • Used for UPDATE (streaming, dynamic tex) │
    // └─────────────────────────────────────────────┴────────────────────────────────────────────┘
    //
    // Key insight:  glTexImage2D  = "allocate + upload" (once at texture creation)
    //               glTexSubImage2D = "upload only" (update existing texture, e.g., video, dynamic UI)
    glTextureSubImage2D(m_id, level, 0, 0, width, height, img->getFormat(), img->getDataType(), img->getData());
}

void Texture::upload(const std::shared_ptr<Image>& img, GLint pos, GLint level) {
    // std::cout << "Uploading texture data from image [" << img->getWidth() << ' ' << img->getHeight() << ' ' << img->getChannels() << ' ' << std::showbase << std::hex << img->getFormat() << ' ' << img->getDataType() << std::dec << "] to level" << level << " GPU memory\n";

    // Check if image data is valid
    if (img == nullptr || img->getData() == nullptr) { throw std::runtime_error("Texture::upload: image data is null"); }
    if (level < 0 || level >= m_mipLevels) { throw std::runtime_error(std::format("Texture::upload: mip level {} out of range [0 - {}]", level, m_mipLevels)); }
    GLsizei width     = img->getWidth();
    GLsizei height    = img->getHeight();
    GLsizei texWidth  = getWidth(level);
    GLsizei texHeight = getHeight(level);
    if (width > texWidth || height > texHeight) { throw std::runtime_error(std::format("Texture::upload: image size {}x{} does not match texture size {}x{} at level {}", width, height, texWidth, texHeight, level)); }

    // Upload texture data to GPU memory
    glTextureSubImage3D(m_id, level, 0, 0, pos, width, height, 1, img->getFormat(), img->getDataType(), img->getData());
}

void Texture::copy(const Texture& other, GLint srcLevel, GLint srcX, GLint srcY, GLint srcZ, GLint dstLevel, GLint dstX, GLint dstY, GLint dstZ) {
    if (dstLevel < 0 || dstLevel >= m_mipLevels || srcLevel < 0 || srcLevel >= other.m_mipLevels) { throw std::runtime_error(std::format("Texture::copy: dst mip level {} out of range [0 - {}] or src mip level {} out of range [0 - {}]", dstLevel, m_mipLevels - 1, srcLevel, other.m_mipLevels - 1)); }

    auto srcId = other.m_id;
    auto dstId = m_id;
    if (srcId == 0 || dstId == 0) { throw std::runtime_error("Texture::copy: texture id is null"); }

    auto srcTarget = other.m_target;
    auto dstTarget = m_target;
    if (srcTarget != dstTarget) { throw std::runtime_error("Texture::copy: source texture target does not match destination texture target"); }

    auto srcWidth  = other.getWidth(srcLevel);
    auto srcHeight = other.getHeight(srcLevel);
    auto dstWidth  = getWidth(dstLevel);
    auto dstHeight = getHeight(dstLevel);
    auto dstDepth  = getDepth(dstLevel);
    if (srcWidth > dstWidth || srcHeight > dstHeight) { throw std::runtime_error(std::format("Texture::copy: source texture size {}x{} does not match destination texture size {}x{} at level {}", srcWidth, srcHeight, dstWidth, dstHeight, dstLevel)); }

    // Copy texture data from source to destination
    // ┌──────────────────────────────────────────────────────────────────────────────────────────┐
    // │                      glCopyTextureSubImage2D vs glCopyImageSubData                        │
    // ├─────────────────────────────────────────────┬────────────────────────────────────────────┤
    // │              glCopyTextureSubImage2D         │           glCopyImageSubData               │
    // ├─────────────────────────────────────────────┼────────────────────────────────────────────┤
    // │ • Source fixed to GL_READ_FRAMEBUFFER color  │ • Source directly texture/RB ID, no FBO req │
    // │ • DSA only for target texture ID             │ • Full DSA: src & dst pass ID directly     │
    // │ • Must bind GL_READ_FRAMEBUFFER global state │ • No global FBO/texture binding side-effect│
    // │ • Only supports FBO → 2D texture sub-region  │ • Arbitrary copy: tex↔tex, tex↔RB, RB↔RB   │
    // │ • Only color buffer copy, no depth/stencil   │ • Supports color / depth / stencil images  │
    // │ • Passes full pixel raster pipeline          │ • Direct GPU memory memcpy, low overhead   │
    // │ • No depth parameter, only 2D slice copy     │ • Supports 1D/2D/3D/array/cubemap layers   │
    // │ • For post-render capture to texture mip     │ • For texture inter-copy, mip layer blit   │
    // └─────────────────────────────────────────────┴────────────────────────────────────────────┘
    //
    // Key insight:  glCopyTextureSubImage2D  = "read from bound FBO color, write to texture (half DSA)"
    //               glCopyImageSubData     = "raw GPU memory copy between any image objects (full stateless DSA)"
    glCopyImageSubData(srcId, srcTarget, srcLevel, srcX, srcY, srcZ, dstId, dstTarget, dstLevel, dstX, dstY, dstZ, srcWidth, srcHeight, dstDepth);
}

void Texture::clamp(GLint level) {
    if (m_id == 0) { return; }
    glTextureParameteri(m_id, GL_TEXTURE_BASE_LEVEL, level);
    glTextureParameteri(m_id, GL_TEXTURE_MAX_LEVEL, level);
}

void Texture::unclamp() {
    if (m_id == 0) { return; }
    glTextureParameteri(m_id, GL_TEXTURE_BASE_LEVEL, 0);
    glTextureParameteri(m_id, GL_TEXTURE_MAX_LEVEL, m_mipLevels - 1);
}

void Texture::generate() {
    if (m_mipLevels > 1) { glGenerateMipmap(m_id); }
}

} // namespace tinyglrenderer