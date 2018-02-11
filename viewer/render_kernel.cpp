
#include "render_kernel.h"
#include <qopenglfunctions.h>
#include <gl/glu.h>
#include <QSizeF>
#include <QMatrix4x4>

void (QOPENGLF_APIENTRYP MultiTexCoord2f)(GLenum target, GLfloat s, GLfloat t);
void *(QOPENGLF_APIENTRYP MapBuffer)(GLenum target, GLenum access);
void (QOPENGLF_APIENTRYP UnmapBuffer)(GLenum target);


render_kernel::render_kernel()
{
    MultiTexCoord2f= 0;
    UnmapBuffer = 0;
    MapBuffer = 0;
}
render_kernel::~render_kernel()
{
}

void render_kernel::viewport(const QRect &r)
{
    glViewport(r.x(), r.y(), r.width(), r.height());
}

void render_kernel::multi_texCoord_2f(const uint target, float s, float t)
{
    if (!MultiTexCoord2f)
    {
        MultiTexCoord2f = reinterpret_cast<void (QOPENGLF_APIENTRYP)
                (GLenum , GLfloat , GLfloat )>
                (QOpenGLContext::currentContext()->getProcAddress("glMultiTexCoord2fARB"));
    }

    MultiTexCoord2f(target, s, t);
}

void *render_kernel::map_buffer(GLenum target, GLenum access)
{
    if (!MapBuffer)
    {
        MapBuffer =
                reinterpret_cast<void *(QOPENGLF_APIENTRYP)
                (GLenum,GLenum)>(QOpenGLContext::currentContext()
                          ->getProcAddress("glMapBufferARB"));
    }
    if (!MapBuffer)
    {
        MapBuffer =
                reinterpret_cast<void *(QOPENGLF_APIENTRYP)
                (GLenum,GLenum)>(QOpenGLContext::currentContext()
                          ->getProcAddress("glMapBufferEXT"));
    }
    return MapBuffer(target, access);
}
void render_kernel::unmap_buffer(GLenum target)
{
    if (!UnmapBuffer)
    {
        UnmapBuffer =
                reinterpret_cast<void (QOPENGLF_APIENTRYP)
                (GLenum)>(QOpenGLContext::currentContext()
                          ->getProcAddress("glUnmapBufferARB"));
    }
    if (!UnmapBuffer)
    {
        UnmapBuffer =
                reinterpret_cast<void (QOPENGLF_APIENTRYP)
                (GLenum)>(QOpenGLContext::currentContext()
                          ->getProcAddress("glUnmapBufferEXT"));
    }
    UnmapBuffer(target);
}

