#ifndef SUBTITLE_HANDEL_H
#define SUBTITLE_HANDEL_H

#include "subtitle_updater.h"
#include "render_kernel.h"

class subtitle_handel :public render_kernel, QOpenGLFunctions
{
private:
    subtitle_renderer _renderer;
    subtitle_updater *_updater;
    subtitle_box _box;

    GLuint _tex;
    bool _tex_current;
    GLuint _fbo;
    GLuint _pbo;
    bool _is_init_opengl;

    class input_handel *input;
    class color_handel *color;
    friend class render_handel;

public:
    subtitle_handel(class input_handel *in, class color_handel *color);
    ~subtitle_handel();

    void handel(const subtitle_box &subtitle);
    void init();
    void deinit();
};

#endif // SUBTITLE_HANDEL_H
