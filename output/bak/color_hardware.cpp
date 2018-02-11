#include "color_hardware.h"
#include "render_hardware.h"

color_hardware::color_hardware():
    hardware()
{
}

void color_hardware::init(int index, const parameters &params, const video_frame &frame)
{
    check_error(HERE);
    glGenFramebuffers(1, &_fbo);
    QString quality_str;
    QString layout_str;
    QString color_space_str;
    QString value_range_str;
    QString storage_str;
    QString chroma_offset_x_str;
    QString chroma_offset_y_str;
    quality_str = QString::number(params.quality());

    if (frame.layout == LAYOUT_BGRA32)
    {
        layout_str = "layout_bgra32";
        color_space_str = "color_space_srgb";
        value_range_str = "value_range_8bit_full";
        storage_str = "storage_srgb";
    }
    else
    {
        layout_str = "layout_yuv_p";
        if (frame.color_space == SPACE_YUV709)
        {
            color_space_str = "color_space_yuv709";
        }
        else
        {
            color_space_str = "color_space_yuv601";
        }
        if (frame.value_range == RANGE_U8_FULL)
        {
            value_range_str = "value_range_8bit_full";
            storage_str = "storage_srgb";
        }
        else if (frame.value_range == RANGE_U8_MPEG)
        {
            value_range_str = "value_range_8bit_mpeg";
            storage_str = "storage_srgb";
        }
        else if (frame.value_range == RANGE_U10_FULL)
        {
            value_range_str = "value_range_10bit_full";
            storage_str = "storage_linear_rgb";
        }
        else
        {
            value_range_str = "value_range_10bit_mpeg";
            storage_str = "storage_linear_rgb";
        }
        chroma_offset_x_str = "0.0";
        chroma_offset_y_str = "0.0";
        if (frame.layout == LAYOUT_YUV422P)
        {
            if (frame.chroma_location == LOCATION_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / static_cast<float>(frame.size.width()
                            / _yuv_chroma_width_divisor));
            }
            else if (frame.chroma_location == LOCATION_TOP_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / static_cast<float>(frame.size.width()
                            / _yuv_chroma_width_divisor));
                chroma_offset_y_str = QString::number(0.5f / static_cast<float>(frame.size.height()
                            / _yuv_chroma_height_divisor));
            }
        }
        else if (frame.layout == LAYOUT_YUV420P)
        {
            if (frame.chroma_location == LOCATION_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / static_cast<float>(frame.size.width()
                            / _yuv_chroma_width_divisor));
            }
            else if (frame.chroma_location == LOCATION_TOP_LEFT)
            {
                chroma_offset_x_str = QString::number(0.5f / static_cast<float>(frame.size.width()
                            / _yuv_chroma_width_divisor));
                chroma_offset_y_str = QString::number(0.5f / static_cast<float>(frame.size.height()
                            / _yuv_chroma_height_divisor));
            }
        }
    }
    if (params.quality() == 0)
    {
        storage_str = "storage_srgb";   // SRGB data in GL_RGB8 texture.
    }
    /*else if (storage_str == "storage_srgb"
            && (!hasOpenGLFeature("GL_EXT_texture_sRGB")
                || getenv("SRGB_TEXTURES_ARE_BROKEN") // XXX: Hack: work around broken SRGB texture implementations
                || !srgb8_textures_are_color_renderable()))
    {
        storage_str = "storage_linear_rgb";
    }*/

    fs_src.replace("$quality", quality_str);
    fs_src.replace("$layout", layout_str);
    fs_src.replace("$color_space", color_space_str);
    fs_src.replace("$value_range", value_range_str);
    fs_src.replace("$chroma_offset_x", chroma_offset_x_str);
    fs_src.replace("$chroma_offset_y", chroma_offset_y_str);
    fs_src.replace("$storage", storage_str);
    _prg[index] = program_create("video_output_color", "", fs_src);
    program_link("video_output_color", _prg[index]);
    for (int i = 0; i < (frame.stereo_layout == STEREO_MONO ? 1 : 2); i++)
    {
        _tex[index][i] = hardware::texture_id(frame.size,
                                          params.quality() == 0  ? GL_RGB
                                          : storage_str == "storage_srgb" ? GL_SRGB8 : GL_RGB16,
                                          GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, GL_LINEAR, GL_LINEAR,
                                          NULL);
    }
    check_error(HERE);
    _last_params[index] = params;
    _last_frame[index] = frame;
}
void color_hardware::deinit(int index)
{
    check_error(HERE);
    glDeleteFramebuffers(1, &_fbo);
    _fbo = 0;
    if (_prg[index] != 0)
    {
        program_delete(_prg[index]);
        _prg[index] = 0;
    }
    for (int i = 0; i < 2; i++)
    {
        if (_tex[index][i] != 0)
        {
            texture_del(_tex[index][i]);
            _tex[index][i] = 0;
        }
    }
    _last_params[index] = parameters();
    _last_frame[index] = video_frame();
    check_error(HERE);
}
bool color_hardware::is_compatible(int index, const parameters& params,
                                   const video_frame &current_frame)
{
    return (_last_params[index].quality() == params.quality()
            && _last_frame[index].size.width() == current_frame.size.width()
            && _last_frame[index].size.height() == current_frame.size.height()
            && _last_frame[index].layout == current_frame.layout
            && _last_frame[index].color_space == current_frame.color_space
            && _last_frame[index].value_range == current_frame.value_range
            && _last_frame[index].chroma_location == current_frame.chroma_location
            && _last_frame[index].stereo_layout == current_frame.stereo_layout);
}
void color_hardware::correction(int index, const video_frame &frame, input_hardware * input)
{
    if (!program_id(index) || !is_compatible(index, ctrl_core::parameter(), frame))
    {
        deinit(index);
        init(index, ctrl_core::parameter(), frame);
    }
    int left = 0;
    int right = (frame.stereo_layout == STEREO_MONO ? 0 : 1);

    // Backup GL state
    GLint framebuffer_bak;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING_EXT, &framebuffer_bak);
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
    glViewport(0, 0, frame.size.width(), frame.size.height());
    glUseProgram(color_hardware::program_id(index));
    if (frame.layout == LAYOUT_BGRA32)
    {
        glUniform1i(glGetUniformLocation(color_hardware::program_id(index), "srgb_tex"), 0);
    }
    else
    {
        glUniform1i(glGetUniformLocation(color_hardware::program_id(index), "y_tex"), 0);
        glUniform1i(glGetUniformLocation(color_hardware::program_id(index), "u_tex"), 1);
        glUniform1i(glGetUniformLocation(color_hardware::program_id(index), "v_tex"), 2);
    }
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, color_hardware::fbo_id());
    // left view: render into _color_tex[index][0]
    if (frame.layout == LAYOUT_BGRA32)
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->bgra32_texture(left));
    }
    else
    {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, input->yuv_y_texture(left));
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, input->yuv_u_texture(left));
        glActiveTexture(GL_TEXTURE2);
        glBindTexture(GL_TEXTURE_2D, input->yuv_v_texture(left));
    }
    glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                           GL_COLOR_ATTACHMENT0_EXT,
                           GL_TEXTURE_2D,
                           color_hardware::texture_id(index,0), 0);
    check_FBO(HERE);
    render_hardware::draw_quad(QRectF(-1.0f, +1.0f, +2.0f, -2.0f));

    if (left != right)
    {
        if (frame.layout == LAYOUT_BGRA32)
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, input->bgra32_texture(right));
        }
        else
        {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, input->yuv_y_texture(right));
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, input->yuv_u_texture(right));
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, input->yuv_v_texture(right));
        }
        glFramebufferTexture2D(GL_FRAMEBUFFER_EXT,
                               GL_COLOR_ATTACHMENT0_EXT,
                               GL_TEXTURE_2D,
                               color_hardware::texture_id(index,1), 0);
        check_FBO(HERE);
        render_hardware::draw_quad(QRectF(-1.0f, +1.0f, +2.0f, -2.0f));
    }

    // Restore GL state
    glBindFramebuffer(GL_FRAMEBUFFER_EXT, framebuffer_bak);
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
