#ifndef GLRENDERTARGET_H
#define GLRENDERTARGET_H

#include <QOpenGLBuffer>
#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>

class GLRenderTarget : protected QOpenGLFunctions {
public:
    GLRenderTarget(int width, int height, int msaa_samples);
    GLRenderTarget(int width,
                   int height,
                   const QOpenGLFramebufferObjectFormat& msfbo_fmt,
                   const QOpenGLFramebufferObjectFormat& ssfbo_fmt);

    bool ok() const { return m_is_ok; }
    size_t buffer_size() const;

    bool bind();
    bool release();
    bool start_pixel_transfer();
    void* map();
    bool unmap();

private:
    QOpenGLFramebufferObject m_multisample_fbo;
    QOpenGLFramebufferObject m_singlesample_fbo;
    QOpenGLBuffer m_pbo;
    bool m_is_ok = true;
    int m_width;
    int m_height;

    static QOpenGLFramebufferObject make_fbo(int width, int height, int samples);
};

#endif // GLRENDERTARGET_H
