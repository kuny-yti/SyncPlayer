#ifndef COLOR_HANDEL_H
#define COLOR_HANDEL_H

#include "video_frame.h"
#include "render_kernel.h"
#include <QOpenGLShaderProgram>

/// 使用着色器
/// 使用fbo
/// 使用纹理对象
/// 作用是将输入的纹理进行颜色转换

class color_handel :public render_kernel, QOpenGLFunctions
{
private:
    class input_handel *input;
    int _last_quality;
    QOpenGLShaderProgram *_prg;
    video_frame _frame;
    GLuint _fbo;
    GLuint _tex[2];
    bool _is_init_opengl;
    QMatrix4x4 color_matrix;

    friend class subtitle_handel;
    friend class render_handel;

    bool srgb8_textures_are();
    bool check_FBO(const QString& where);
    void draw_quad(float x, float y, float w, float h,
            const float tex_coords[2][4][2] = NULL,
            const float more_tex_coords[4][2] = NULL) ;
public:
    color_handel(class input_handel *in);
    ~color_handel();

    void handel(const video_frame &frame);
    void draw(const video_frame &frame);
//protected:
    void init(int quality = 4);
    void deinit();
    bool is_compatible(int quality, const video_frame &frame);

};

#endif // COLOR_HANDEL_H
