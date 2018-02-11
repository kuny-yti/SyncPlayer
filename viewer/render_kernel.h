#ifndef RENDER_KERNEL_H
#define RENDER_KERNEL_H
#include "type_define.h"
#include <QSizeF>
#include <QOpenGLFunctions>

class render_kernel
{
public:
    render_kernel();
    virtual ~render_kernel();

    virtual void viewport(const QRect &r);
    virtual void multi_texCoord_2f(const uint target, float s, float t);
    void *map_buffer(GLenum target, GLenum access);
    void unmap_buffer(GLenum target);

};

#endif // RENDER_KERNEL_H
