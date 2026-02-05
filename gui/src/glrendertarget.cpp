#include "glrendertarget.h"

#include <cstddef>

GLRenderTarget::GLRenderTarget(int width, int height, int msaa_samples)
    : m_multisample_fbo(make_fbo(width, height, msaa_samples)),
      m_singlesample_fbo(make_fbo(width, height, 0)),
      m_pbo(QOpenGLBuffer::PixelPackBuffer),
      m_width(width),
      m_height(height) {

    initializeOpenGLFunctions();
    if (!m_pbo.create()) {
        m_is_ok = false;
        return;
    }

    m_pbo.setUsagePattern(QOpenGLBuffer::DynamicRead);
}

size_t GLRenderTarget::buffer_size() const {
    return static_cast<size_t>(m_width) * static_cast<size_t>(m_height) * 4;
}

bool GLRenderTarget::bind() {
    return m_multisample_fbo.bind();
}

bool GLRenderTarget::release() {
    return m_multisample_fbo.release();
}

bool GLRenderTarget::start_pixel_transfer() {
    // Blit pixels to singlesample FBO w/ linear interpolation
    QOpenGLFramebufferObject::blitFramebuffer(&m_singlesample_fbo,
                                              &m_multisample_fbo,
                                              GL_COLOR_BUFFER_BIT,
                                              GL_LINEAR);

    // Bind FBO and PBO, allocate PBO
    if (!m_singlesample_fbo.bind())
        return false;
    if (!m_pbo.bind())
        return false;
    m_pbo.allocate(buffer_size());

    // Initiate pixel transfer from FBO to PBO
    glReadPixels(0, 0, m_width, m_height, GL_RGBA, GL_UNSIGNED_BYTE, 0);
    return true;
}

void* GLRenderTarget::map() {
    m_pbo.bind();
    return m_pbo.map(QOpenGLBuffer::ReadOnly);
}

bool GLRenderTarget::unmap() {
    bool ret = m_pbo.unmap();
    m_pbo.release();
    return ret;
}

QOpenGLFramebufferObject GLRenderTarget::make_fbo(int width, int height, int samples) {
    QOpenGLFramebufferObjectFormat fbo_format;
    fbo_format.setAttachment(QOpenGLFramebufferObject::CombinedDepthStencil);
    fbo_format.setSamples(samples);
    fbo_format.setInternalTextureFormat(GL_RGBA);
    return QOpenGLFramebufferObject(width, height, fbo_format);
}
