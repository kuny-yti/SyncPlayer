#include "input_hardware.h"

static int next_multiple_of_4(int x)
{
    return (x / 4 + (x % 4 == 0 ? 0 : 1)) * 4;
}

input_hardware::input_hardware():
    hardware()
{
}
void input_hardware::init(const video_frame &frame)
{
    check_error(HERE);
    glGenBuffers(1, &_pbo);
    glGenFramebuffers(1, &_fbo);
    if (frame.layout == LAYOUT_BGRA32)
    {
        for (int i = 0; i < (frame.stereo_layout == STEREO_MONO ? 1 : 2); i++)
        {
            _bgra32_tex[i] = texture_id(frame.size,
                                              GL_RGB8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                                              GL_NEAREST, GL_NEAREST,
                                              NULL);
        }
    }
    else
    {
        _yuv_chroma_width_divisor = 1;
        _yuv_chroma_height_divisor = 1;
        bool need_chroma_filtering = false;
        if (frame.layout == LAYOUT_YUV422P)
        {
            _yuv_chroma_width_divisor = 2;
            need_chroma_filtering = true;
        }
        else if (frame.layout == LAYOUT_YUV420P)
        {
            _yuv_chroma_width_divisor = 2;
            _yuv_chroma_height_divisor = 2;
            need_chroma_filtering = true;
        }
        bool type_u8 = (frame.value_range == RANGE_U8_FULL || frame.value_range == RANGE_U8_MPEG);
        GLint internal_format = type_u8 ? GL_LUMINANCE8 : GL_LUMINANCE16;
        GLint type = type_u8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
        for (int i = 0; i < (frame.stereo_layout == STEREO_MONO ? 1 : 2); i++)
        {
            _yuv_y_tex[i] = texture_id(frame.size,
                                              internal_format, GL_LUMINANCE, type,
                                              GL_NEAREST, GL_NEAREST,
                                              NULL);
            QSize size = QSize(frame.size.width() / _yuv_chroma_width_divisor,
                               frame.size.height() / _yuv_chroma_height_divisor);
            _yuv_u_tex[i] = texture_id(size,
                                             internal_format, GL_LUMINANCE, type,
                                             need_chroma_filtering ? GL_LINEAR : GL_NEAREST,
                                             need_chroma_filtering ? GL_LINEAR : GL_NEAREST,
                                             NULL);
            _yuv_v_tex[i] = texture_id(size,
                                             internal_format, GL_LUMINANCE, type,
                                             need_chroma_filtering ? GL_LINEAR : GL_NEAREST,
                                             need_chroma_filtering ? GL_LINEAR : GL_NEAREST,
                                             NULL);
        }
    }
}
void input_hardware::deinit()
{
    check_error(HERE);
    glDeleteBuffers(1, &_pbo);
    _pbo = 0;
    glDeleteFramebuffers(1, &_fbo);
    _fbo = 0;
    for (int i = 0; i < 2; i++)
    {
        if (_yuv_y_tex[i] != 0)
        {
            texture_del(_yuv_y_tex[i]);
            _yuv_y_tex[i] = 0;
        }
        if (_yuv_u_tex[i] != 0)
        {
            texture_del(_yuv_u_tex[i]);
            _yuv_u_tex[i] = 0;
        }
        if (_yuv_v_tex[i] != 0)
        {
            texture_del(_yuv_v_tex[i]);
            _yuv_v_tex[i] = 0;
        }
        if (_bgra32_tex[i] != 0)
        {
            texture_del(_bgra32_tex[i]);
            _bgra32_tex[i] = 0;
        }
    }
    _yuv_chroma_width_divisor = 0;
    _yuv_chroma_height_divisor = 0;
    _last_frame = video_frame();
    check_error(HERE);
}
bool input_hardware::is_compatible(const video_frame &current_frame)
{
    return (_last_frame.size.width() == current_frame.size.width()
            && _last_frame.size.height() == current_frame.size.height()
            && _last_frame.layout == current_frame.layout
            && _last_frame.color_space == current_frame.color_space
            && _last_frame.value_range == current_frame.value_range
            && _last_frame.chroma_location == current_frame.chroma_location
            && _last_frame.stereo_layout == current_frame.stereo_layout);
}
void input_hardware::upload_frame(const video_frame &frame)
{
    if (!is_compatible(frame))
    {
        deinit();
        init(frame);
        last_frame() = frame;
    }

    int bytes_per_pixel = 4;
    GLenum format = GL_BGRA;
    GLenum type = GL_UNSIGNED_INT_8_8_8_8_REV;
    if (frame.layout != LAYOUT_BGRA32)
    {
        bool type_u8 = (frame.value_range == RANGE_U8_FULL || frame.value_range == RANGE_U8_MPEG);
        bytes_per_pixel = type_u8 ? 1 : 2;
        format = GL_LUMINANCE;
        type = type_u8 ? GL_UNSIGNED_BYTE : GL_UNSIGNED_SHORT;
    }

    for (int i = 0; i < (frame.stereo_layout == STEREO_MONO ? 1 : 2); i++)
    {
        for (int plane = 0; plane < (frame.layout == LAYOUT_BGRA32 ? 1 : 3); plane++)
        {
            int w = frame.size.width();
            int h = frame.size.height();
            GLuint tex;
            int row_size;
            if (frame.layout == LAYOUT_BGRA32)
            {
                tex = bgra32_texture(i);
            }
            else
            {
                if (plane != 0)
                {
                    w /= yuv_chroma_width();
                    h /= yuv_chroma_height();
                }
                tex = (plane == 0 ? yuv_y_texture(i)
                                    : plane == 1 ? yuv_u_texture(i)
                                                   : yuv_v_texture(i));
            }
            row_size = next_multiple_of_4(w * bytes_per_pixel);

            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, pbo_id());
            glBufferData(GL_PIXEL_UNPACK_BUFFER, row_size * h, NULL, GL_STREAM_DRAW);
            void *pboptr = glMapBuffer(GL_PIXEL_UNPACK_BUFFER, GL_WRITE_ONLY);
            if (!pboptr)
                throw QString("Cannot create a PBO buffer.");
            Q_ASSERT(reinterpret_cast<uintptr_t>(pboptr) % 4 == 0);

            frame.copy_plane(i, plane, pboptr);

            glUnmapBuffer(GL_PIXEL_UNPACK_BUFFER);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, row_size / bytes_per_pixel);
            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, tex);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, format, type, NULL);
            glBindBuffer(GL_PIXEL_UNPACK_BUFFER, 0);
        }
    }
}
