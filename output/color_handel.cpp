#include "color_handel.h"
#include "input_handel.h"

#include <QFile>


static void matmult(const float A[16], const float B[16], float result[16])
{
    for (int r = 0; r < 4; r++) {
        for (int c = 0; c < 4; c++) {
            result[r * 4 + c] =
                  A[r * 4 + 0] * B[0 * 4 + c]
                + A[r * 4 + 1] * B[1 * 4 + c]
                + A[r * 4 + 2] * B[2 * 4 + c]
                + A[r * 4 + 3] * B[3 * 4 + c];
        }
    }
}

void get_color_matrix(float brightness, float contrast, float hue, float saturation, float matrix[16])
{
    /*
     * See http://www.graficaobscura.com/matrix/index.html for the basic ideas.
     * Note that the hue matrix is computed in a simpler way.
     */

    const float wr = 0.3086f;
    const float wg = 0.6094f;
    const float wb = 0.0820f;

    // Brightness and Contrast
    float b = (brightness < 0.0f ? brightness : 4.0f * brightness) + 1.0f;
    float c = -contrast;
    float BC[16] = {
           b, 0.0f, 0.0f, 0.0f,
        0.0f,    b, 0.0f, 0.0f,
        0.0f, 0.0f,    b, 0.0f,
           c,    c,    c, 1.0f
    };
    // Saturation
    float s = saturation + 1.0f;
    float S[16] = {
        (1.0f - s) * wr + s, (1.0f - s) * wg    , (1.0f - s) * wb    , 0.0f,
        (1.0f - s) * wr    , (1.0f - s) * wg + s, (1.0f - s) * wb    , 0.0f,
        (1.0f - s) * wr    , (1.0f - s) * wg    , (1.0f - s) * wb + s, 0.0f,
                       0.0f,                0.0f,                0.0f, 1.0f
    };
    // Hue
    float n = 1.0f / sqrtf(3.0f);       // normalized hue rotation axis: sqrt(3) * (1 1 1)
    float h = hue * M_PI;               // hue rotation angle
    float hc = cosf(h);
    float hs = sinf(h);
    float H[16] = {     // conversion of angle/axis representation to matrix representation
        n * n * (1.0f - hc) + hc    , n * n * (1.0f - hc) - n * hs, n * n * (1.0f - hc) + n * hs, 0.0f,
        n * n * (1.0f - hc) + n * hs, n * n * (1.0f - hc) + hc    , n * n * (1.0f - hc) - n * hs, 0.0f,
        n * n * (1.0f - hc) - n * hs, n * n * (1.0f - hc) + n * hs, n * n * (1.0f - hc) + hc    , 0.0f,
                                0.0f,                         0.0f,                         0.0f, 1.0f
    };

    /* I
    float I[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    */

    // matrix = B * C * S * H
    float T[16];
    matmult(BC, S, T);
    matmult(T, H, matrix);
}



static const float full_tex_coords[2][4][2] =
{
    { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } }
};
void color_handel::draw_quad(float x, float y, float w, float h,
        const float tex_coords[2][4][2],
        const float more_tex_coords[4][2])
{
    const float (*my_tex_coords)[4][2] = (tex_coords ? tex_coords : full_tex_coords);

    glBegin(GL_QUADS);
    glTexCoord2f(my_tex_coords[0][0][0], my_tex_coords[0][0][1]);
    multi_texCoord_2f(GL_TEXTURE1, my_tex_coords[1][0][0], my_tex_coords[1][0][1]);
    if (more_tex_coords) {
        multi_texCoord_2f(GL_TEXTURE2, more_tex_coords[0][0], more_tex_coords[0][1]);
    }
    glVertex2f(x, y);
    glTexCoord2f(my_tex_coords[0][1][0], my_tex_coords[0][1][1]);
    multi_texCoord_2f(GL_TEXTURE1, my_tex_coords[1][1][0], my_tex_coords[1][1][1]);
    if (more_tex_coords) {
        multi_texCoord_2f(GL_TEXTURE2, more_tex_coords[1][0], more_tex_coords[1][1]);
    }
    glVertex2f(x + w, y);
    glTexCoord2f(my_tex_coords[0][2][0], my_tex_coords[0][2][1]);
    multi_texCoord_2f(GL_TEXTURE1, my_tex_coords[1][2][0], my_tex_coords[1][2][1]);
    if (more_tex_coords) {
        multi_texCoord_2f(GL_TEXTURE2, more_tex_coords[2][0], more_tex_coords[2][1]);
    }
    glVertex2f(x + w, y + h);
    glTexCoord2f(my_tex_coords[0][3][0], my_tex_coords[0][3][1]);
    multi_texCoord_2f(GL_TEXTURE1, my_tex_coords[1][3][0], my_tex_coords[1][3][1]);
    if (more_tex_coords) {
        multi_texCoord_2f(GL_TEXTURE2, more_tex_coords[3][0], more_tex_coords[3][1]);
    }
    glVertex2f(x, y + h);
    glEnd();
}

color_handel::color_handel(input_handel *in):
    input(in)
{
    _is_init_opengl = false;
    _frame = video_frame();
    _prg = 0;

    _fbo = 0;
    _tex[0] = 0;
    _tex[1] = 0;
    color_matrix.data();
    get_color_matrix(0.0, 0.0, 0.0, 0.0, color_matrix.data());
}
color_handel::~color_handel()
{
    deinit();
}

void color_handel::draw(const video_frame &frame)
{
    GLint viewport_bak[4];
    glGetIntegerv(GL_VIEWPORT, viewport_bak);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    QRect gemo= QRect(viewport_bak[0],viewport_bak[1],
            viewport_bak[2], viewport_bak[3]);

    float h_ = ((float)viewport_bak[2] / frame.raw_aspect_ratio);//根据当前视口的宽度计算同比例的视口高度
    float raw_ar = (float)frame.raw_size.width () / (float)frame.raw_size.height ();
    if (frame.raw_aspect_ratio != raw_ar)
        h_ = ((float)viewport_bak[2] / raw_ar);

    if (h_ > viewport_bak[3])
    {
        float w_ = viewport_bak[3] *frame.raw_aspect_ratio;
        gemo.setWidth(w_);
        gemo.setX((viewport_bak[2]-w_)/2);
        viewport(QRect((viewport_bak[2]-w_)/2
                 ,0,w_, viewport_bak[3]));
    }
    else
    {
        gemo.setHeight(h_);
        gemo.setY((viewport_bak[3]-h_) /2);
        viewport(QRect(0,(viewport_bak[3]-h_) /2,viewport_bak[2], h_));
    }

    float x = -1.0 ;
    float y = -1.0 ;
    float w = 2.0f;
    float h = 2.0 ;

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, _tex[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 0);
    glVertex3f(x, y, 0);

    glTexCoord2f(1, 0);
    glVertex3f(x+w, y, 0);

    glTexCoord2f(1, 1);
    glVertex3f(x+w, y+h, 0);

    glTexCoord2f(0, 1);
    glVertex3f(x, y+h, 0);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    glViewport(viewport_bak[0], viewport_bak[1], viewport_bak[2], viewport_bak[3]);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
}
void color_handel::handel(const video_frame &frame)
{
    if (!input)
        return;

    _frame = frame;
    if (!_prg || !is_compatible(4, frame))
    {
        deinit();
        init();
    }
    int left = 0;
    int right = (frame.stereo_layout == STEREO_MONO ? 0 : 1);

    // Backup GL state
    GLint framebuffer_bak;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer_bak);
    GLint viewport_bak[4];
    glGetIntegerv(GL_VIEWPORT, viewport_bak);
    GLboolean scissor_bak = glIsEnabled(GL_SCISSOR_TEST);

    glMatrixMode(GL_MODELVIEW);
    glPushMatrix();
    glMatrixMode(GL_PROJECTION);
    glPushMatrix();

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
    glDisable(GL_SCISSOR_TEST);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glViewport(0, 0, frame.raw_size.width(), frame.raw_size.height());

    _prg->bind();
    _prg->setUniformValue("color_matrix", color_matrix);
    if (frame.layout == LAYOUT_BGRA32)
    {
        _prg->setUniformValue("srgb_tex", 0);
    }
    else
    {
        _prg->setUniformValue("y_tex", 0);
        _prg->setUniformValue("u_tex", 1);
        _prg->setUniformValue("v_tex", 2);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, _fbo);
    if (frame.layout == LAYOUT_BGRA32)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->_bgra32_tex[left]);
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->_yuv_y_tex[left]);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, input->_yuv_u_tex[left]);
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, input->_yuv_v_tex[left]);
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                           _tex[0], 0);
    //check_FBO((HERE));
    draw_quad(-1.0f, +1.0f, +2.0f, -2.0f);
    if (left != right)
    {
        if (frame.layout == LAYOUT_BGRA32) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, input->_bgra32_tex[right]);
        } else {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, input->_yuv_y_tex[right]);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, input->_yuv_u_tex[right]);
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, input->_yuv_v_tex[right]);
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                               _tex[1], 0);
        //check_FBO((HERE));
        draw_quad(-1.0f, +1.0f, +2.0f, -2.0f);
    }
    _prg->release();
    // Restore GL state
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_bak);
    if (scissor_bak)
        glEnable(GL_SCISSOR_TEST);
    glViewport(viewport_bak[0], viewport_bak[1], viewport_bak[2], viewport_bak[3]);
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);
}

void color_handel::init(int quality)
{
    if (!_is_init_opengl)
    {
        initializeOpenGLFunctions();
        _is_init_opengl = true;
    }
    glGenFramebuffers(1, &_fbo);

    QString quality_str;
    QString layout_str;
    QString color_space_str;
    QString value_range_str;
    QString storage_str;
    QString chroma_offset_x_str;
    QString chroma_offset_y_str;

    quality_str = QString::number(quality);
    if (input->_last_frame.layout == LAYOUT_BGRA32)
    {
        layout_str = "layout_bgra32";
        color_space_str = "color_space_srgb";
        value_range_str = "value_range_8bit_full";
        storage_str = "storage_srgb";
    }
    else
    {
        layout_str = "layout_yuv_p";
        if (input->_last_frame.color_space == SPACE_YUV709) {
            color_space_str = "color_space_yuv709";
        } else {
            color_space_str = "color_space_yuv601";
        }
        if (input->_last_frame.value_range == RANGE_U8_FULL) {
            value_range_str = "value_range_8bit_full";
            storage_str = "storage_srgb";
        } else if (input->_last_frame.value_range == RANGE_U8_MPEG) {
            value_range_str = "value_range_8bit_mpeg";
            storage_str = "storage_srgb";
        } else if (input->_last_frame.value_range == RANGE_U10_FULL) {
            value_range_str = "value_range_10bit_full";
            storage_str = "storage_linear_rgb";
        } else {
            value_range_str = "value_range_10bit_mpeg";
            storage_str = "storage_linear_rgb";
        }
        chroma_offset_x_str = "0.0";
        chroma_offset_y_str = "0.0";
        if (input->_last_frame.layout == LAYOUT_YUV422P)
        {
            if (input->_last_frame.chroma_location == LOCATION_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / float(input->_last_frame.size.width()
                            / input->_yuv_chroma.width()));
            }
            else if (input->_last_frame.chroma_location == LOCATION_TOP_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / float(input->_last_frame.size.width()
                            / input->_yuv_chroma.width()));
                chroma_offset_y_str = QString::number(0.5f / float(input->_last_frame.size.height()
                            / input->_yuv_chroma.height()));
            }
        }
        else if (input->_last_frame.layout == LAYOUT_YUV420P)
        {
            if (input->_last_frame.chroma_location == LOCATION_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / float(input->_last_frame.size.width()
                            / input->_yuv_chroma.width()));
            } else if (input->_last_frame.chroma_location == LOCATION_TOP_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / float(input->_last_frame.size.width()
                            / input->_yuv_chroma.width()));
                chroma_offset_y_str = QString::number(0.5f / float(input->_last_frame.size.height()
                            / input->_yuv_chroma.height()));
            }
        }
    }

    if (quality == 0)
    {
        storage_str = "storage_srgb";   // SRGB data in GL_RGB8 texture.
    }
    else if (storage_str == "storage_srgb" && (!srgb8_textures_are()))
    {
        storage_str = "storage_linear_rgb";
    }

    QString color;
    QFile file(":/shader/video_output_color.fs.glsl");
    if (file.open(QIODevice::ReadOnly))
    {
        color = QString(file.readAll());
    }
    file.close();

    QString color_fs_src(color.toLatin1().data());
    color_fs_src.replace("$quality", quality_str);
    color_fs_src.replace("$layout", layout_str);
    color_fs_src.replace("$color_space", color_space_str);
    color_fs_src.replace("$value_range", value_range_str);
    color_fs_src.replace("$chroma_offset_x", chroma_offset_x_str);
    color_fs_src.replace("$chroma_offset_y", chroma_offset_y_str);
    color_fs_src.replace("$storage", storage_str);

    if (!_prg)
    {
        _prg = new QOpenGLShaderProgram();
        _prg->addShaderFromSourceCode(QOpenGLShader::Fragment,
                                              color_fs_src);
        _prg->link();
    }

    for (int i = 0; i < (input->_last_frame.stereo_layout == STEREO_MONO ? 1 : 2); i++)
    {
        glGenTextures(1, &(_tex[i]));
        glBindTexture(GL_TEXTURE_2D, _tex[i]);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexImage2D(GL_TEXTURE_2D, 0,
                quality == 0 ? GL_RGB
                : storage_str == "storage_srgb" ? GL_SRGB8 : GL_RGB16,
                input->_last_frame.raw_size.width(), input->_last_frame.raw_size.height(),
                     0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    }
    _last_quality = quality;
}
void color_handel::deinit()
{
    if (_fbo)
        glDeleteFramebuffers(1, &_fbo);

    if (_prg != 0)
    {
        delete _prg;
    }
    for (int i = 0; i < 2; i++)
    {
        if (_tex[i] != 0)
        {
            glDeleteTextures(1, &(_tex[i]));
        }
    }
}
bool color_handel::is_compatible(int quality, const video_frame &frame)
{
    if (!input)
        return false;
    return (_last_quality == quality && input->is_compatible(frame));

}
bool color_handel::check_FBO(const QString& where)
{
    GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (e != GL_FRAMEBUFFER_COMPLETE)
    {
        QString pfx = (where.length() > 0 ? where + ": " : "");
        DEBUG (pfx + QString("OpenGL Framebuffer status error 0x")
                  +QString::number(e, 16));
        return false;
    }
    return true;
}
bool color_handel::srgb8_textures_are()
{
    bool retval = true;
    GLuint fbo;
    GLuint tex;

    glGenFramebuffers(1, &fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, 2, 2, 0, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, NULL);
    GLint framebuffer_bak;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &framebuffer_bak);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);
    GLenum e = glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (e != GL_FRAMEBUFFER_COMPLETE) {
        retval = false;
    }
    glBindFramebuffer(GL_FRAMEBUFFER, framebuffer_bak);
    glDeleteFramebuffers(1, &fbo);
    glDeleteTextures(1, &tex);
    return retval;
}
