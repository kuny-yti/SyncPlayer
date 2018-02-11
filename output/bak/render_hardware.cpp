#include "render_hardware.h"
#include <cmath>

void get_color_matrix(float brightness, float contrast, float hue, float saturation, float matrix[16]);
render_hardware::render_hardware():
    hardware()
{
}
void render_hardware::init()
{
    check_error(HERE);
    QString quality_str = QString::number(_params.quality());
    QString mode_str = (
            _params.stereo_mode() == MODE_EVEN_ODD_ROWS ? "mode_even_odd_rows"
            : _params.stereo_mode() == MODE_EVEN_ODD_COLUMNS ? "mode_even_odd_columns"
            : _params.stereo_mode() == MODE_CHECKERBOARD ? "mode_checkerboard"
            : _params.stereo_mode() == MODE_RED_CYAN_MONOCHROME ? "mode_red_cyan_monochrome"
            : _params.stereo_mode() == MODE_RED_CYAN_HALF_COLOR ? "mode_red_cyan_half_color"
            : _params.stereo_mode() == MODE_RED_CYAN_FULL_COLOR ? "mode_red_cyan_full_color"
            : _params.stereo_mode() == MODE_RED_CYAN_DUBOIS ? "mode_red_cyan_dubois"
            : _params.stereo_mode() == MODE_GREEN_MAGENTA_MONOCHROME ? "mode_green_magenta_monochrome"
            : _params.stereo_mode() == MODE_GREEN_MAGENTA_HALF_COLOR ? "mode_green_magenta_half_color"
            : _params.stereo_mode() == MODE_GREEN_MAGENTA_FULL_COLOR ? "mode_green_magenta_full_color"
            : _params.stereo_mode() == MODE_GREEN_MAGENTA_DUBOIS ? "mode_green_magenta_dubois"
            : _params.stereo_mode() == MODE_AMBER_BLUE_MONOCHROME ? "mode_amber_blue_monochrome"
            : _params.stereo_mode() == MODE_AMBER_BLUE_HALF_COLOR ? "mode_amber_blue_half_color"
            : _params.stereo_mode() == MODE_AMBER_BLUE_FULL_COLOR ? "mode_amber_blue_full_color"
            : _params.stereo_mode() == MODE_AMBER_BLUE_DUBOIS ? "mode_amber_blue_dubois"
            : _params.stereo_mode() == MODE_RED_GREEN_MONOCHROME ? "mode_red_green_monochrome"
            : _params.stereo_mode() == MODE_RED_BLUE_MONOCHROME ? "mode_red_blue_monochrome"
            : "mode_onechannel");
    QString subtitle_str = (needs_subtitle(_params) ? "subtitle_enabled" : "subtitle_disabled");
    QString coloradjust_str = (needs_coloradjust(_params) ? "coloradjust_enabled" : "coloradjust_disabled");
    QString ghostbust_str = (needs_ghostbust(_params) ? "ghostbust_enabled" : "ghostbust_disabled");

    fs_src.replace("$quality", quality_str);
    fs_src.replace("$mode", mode_str);
    fs_src.replace("$subtitle", subtitle_str);
    fs_src.replace("$coloradjust", coloradjust_str);
    fs_src.replace( "$ghostbust", ghostbust_str);
    _prg = program_create("render_hardware_render", "", fs_src);
    program_link("render_hardware_render", _prg);
    quint32 dummy_texture = 0;

    _dummy_tex = hardware::texture_id(QSize(1,1), GL_RGBA8, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV,
                                   GL_NEAREST,GL_NEAREST,
                                   (uchar*)&dummy_texture);

    if (_params.stereo_mode() == MODE_EVEN_ODD_ROWS
            || _params.stereo_mode() == MODE_EVEN_ODD_COLUMNS
            || _params.stereo_mode() == MODE_CHECKERBOARD)
    {
        GLubyte even_odd_rows_mask[4] = { 0xff, 0xff, 0x00, 0x00 };
        GLubyte even_odd_columns_mask[4] = { 0xff, 0x00, 0xff, 0x00 };
        GLubyte checkerboard_mask[4] = { 0xff, 0x00, 0x00, 0xff };

        _mask_tex = texture_id(QSize(2, 2), GL_LUMINANCE8, GL_LUMINANCE, GL_UNSIGNED_BYTE,
                                      GL_NEAREST,GL_NEAREST,
                                      _params.stereo_mode() == MODE_EVEN_ODD_ROWS ? even_odd_rows_mask
                                      : _params.stereo_mode() == MODE_EVEN_ODD_COLUMNS ? even_odd_columns_mask
                                      : checkerboard_mask);

    }
    check_error(HERE);
}
void render_hardware::deinit()
{
    check_error(HERE);
    if (_prg != 0) {
        program_delete(_prg);
        _prg = 0;
    }
    if (_dummy_tex != 0) {
        texture_del(_dummy_tex);
        _dummy_tex = 0;
    }
    if (_mask_tex != 0) {
        texture_del(_mask_tex);
        _mask_tex = 0;
    }
    _last_params = parameters();
    _last_frame = video_frame();
    check_error(HERE);
}
bool render_hardware::needs_subtitle(const parameters& params)
{
    return params.subtitle_stream() != -1;
}

bool render_hardware::needs_coloradjust(const parameters& params)
{
    return params.contrast() < 0.0f || params.contrast() > 0.0f
        || params.brightness() < 0.0f || params.brightness() > 0.0f
        || params.hue() < 0.0f || params.hue() > 0.0f
        || params.saturation() < 0.0f || params.saturation() > 0.0f;
}

bool render_hardware::needs_ghostbust(const parameters& params)
{
    return params.ghostbust() > 0.0f;
}

bool render_hardware::is_compatible()
{
    return (_last_params.quality() == _params.quality()
            && _last_params.stereo_mode() == _params.stereo_mode()
            && needs_subtitle(_last_params) == needs_subtitle(_params)
            && needs_coloradjust(_last_params) == needs_coloradjust(_params)
            && needs_ghostbust(_last_params) == needs_ghostbust(_params));
}

static const float full_tex_coords[2][4][2] =
{
    { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } }
};
void render_hardware::draw_quad(const QRectF &rect,
        const float tex_coords[2][4][2],
        const float more_tex_coords[4][2])
{
    const float (*my_tex_coords)[4][2] = (tex_coords ? tex_coords : full_tex_coords);

    glBegin(GL_QUADS);
    glTexCoord2f(my_tex_coords[0][0][0], my_tex_coords[0][0][1]);
    glMultiTexCoord2f(GL_TEXTURE1, my_tex_coords[1][0][0], my_tex_coords[1][0][1]);
    if (more_tex_coords)
    {
        glMultiTexCoord2f(GL_TEXTURE2, more_tex_coords[0][0], more_tex_coords[0][1]);
    }
    glVertex2f(rect.x (), rect.y ());
    glTexCoord2f(my_tex_coords[0][1][0], my_tex_coords[0][1][1]);
    glMultiTexCoord2f(GL_TEXTURE1, my_tex_coords[1][1][0], my_tex_coords[1][1][1]);
    if (more_tex_coords)
    {
        glMultiTexCoord2f(GL_TEXTURE2, more_tex_coords[1][0], more_tex_coords[1][1]);
    }
    glVertex2f(rect.x () + rect.width (), rect.y ());
    glTexCoord2f(my_tex_coords[0][2][0], my_tex_coords[0][2][1]);
    glMultiTexCoord2f(GL_TEXTURE1, my_tex_coords[1][2][0], my_tex_coords[1][2][1]);
    if (more_tex_coords) {
        glMultiTexCoord2f(GL_TEXTURE2, more_tex_coords[2][0], more_tex_coords[2][1]);
    }
    glVertex2f(rect.x () + rect.width (), rect.y () + rect.height ());
    glTexCoord2f(my_tex_coords[0][3][0], my_tex_coords[0][3][1]);
    glMultiTexCoord2f(GL_TEXTURE1, my_tex_coords[1][3][0], my_tex_coords[1][3][1]);
    if (more_tex_coords)
    {
        glMultiTexCoord2f(GL_TEXTURE2, more_tex_coords[3][0], more_tex_coords[3][1]);
    }
    glVertex2f(rect.x (), rect.y () + rect.height ());
    glEnd();
}

void render_hardware::draw_window_pos(const display_data &data)
{
    if (params().fullscreen() && params().fullscreen_3d_ready_sync()
            && (params().stereo_mode() == MODE_LEFT_RIGHT
                || params().stereo_mode() == MODE_LEFT_RIGHT_HALF
                || params().stereo_mode() == MODE_TOP_BOTTOM
                || params().stereo_mode() == MODE_TOP_BOTTOM_HALF
                || params().stereo_mode() == MODE_ALTERNATING))
    {
        const quint32 R = 0xffu << 16u;
        const quint32 G = 0xffu << 8u;
        const quint32 B = 0xffu;

        size_t req_size = data.screen.width() * sizeof(quint32);
        if (ready_sync_3d().size() < req_size)
            ready_sync_3d().resize(req_size);

        if (params().stereo_mode() == MODE_LEFT_RIGHT
                || params().stereo_mode() == MODE_LEFT_RIGHT_HALF)
        {
            quint32 color = (data.display_frameno % 2 == 0 ? R : G | B);
            for (int i = 0; i < data.screen.width(); i++)
                ready_sync_3d().ptr<quint32>()[i] = color;
            glWindowPos2i(0, 0);
            glDrawPixels(data.screen.width(), 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ready_sync_3d().ptr());
        }
        else if (params().stereo_mode() == MODE_TOP_BOTTOM
                   || params().stereo_mode() == MODE_TOP_BOTTOM_HALF)
        {
            quint32 color = (data.display_frameno % 2 == 0 ? B : R | G);
            for (int i = 0; i < data.screen.width(); i++)
                ready_sync_3d().ptr<quint32>()[i] = color;

            glWindowPos2i(0, 0);
            glDrawPixels(data.screen.width(), 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ready_sync_3d().ptr());
            glWindowPos2i(0, data.screen.height() / 2);
            glDrawPixels(data.screen.width(), 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ready_sync_3d().ptr());
        }
        else if (params().stereo_mode() == MODE_ALTERNATING)
        {
            quint32 color = (data.display_frameno % 4 < 2 ? G : R | B);
            for (int i = 0; i < data.screen.width(); i++)
                ready_sync_3d().ptr<quint32>()[i] = color;

            glWindowPos2i(0, 0);
            glDrawPixels(data.screen.width(), 1, GL_BGRA, GL_UNSIGNED_INT_8_8_8_8_REV, ready_sync_3d().ptr());
        }
    }
}
void render_hardware::draw_stereo(const display_data &data, const float my_tex_coords[2][4][2])
{
    glViewport(data.viewport[0][0], data.viewport[0][1], data.viewport[0][2], data.viewport[0][3]);
    if (params().stereo_mode() == MODE_STEREO)
    {
        glUniform1f(glGetUniformLocation(program_id (), "channel"), 0.0f);
        glDrawBuffer(GL_BACK_LEFT);
        draw_quad(data.rect, my_tex_coords);
        glUniform1f(glGetUniformLocation(program_id (), "channel"), 1.0f);
        glDrawBuffer(GL_BACK_RIGHT);
        draw_quad(data.rect, my_tex_coords);
    }
    else if (params().stereo_mode() == MODE_EVEN_ODD_ROWS
               || params().stereo_mode() == MODE_EVEN_ODD_COLUMNS
               || params().stereo_mode() == MODE_CHECKERBOARD)
    {
        float vpw = static_cast<float>(data.viewport[0][2]);
        float vph = static_cast<float>(data.viewport[0][3]);
        float more_tex_coords[4][2] =
        {
            { 0.0f, 0.0f }, { vpw / 2.0f, 0.0f }, { vpw / 2.0f, vph / 2.0f }, { 0.0f, vph / 2.0f }
        };
        glActiveTexture(GL_TEXTURE3);
        glBindTexture(GL_TEXTURE_2D, _mask_tex);
        draw_quad(data.rect, my_tex_coords, more_tex_coords);
    }
    else if (params().stereo_mode() == MODE_RED_GREEN_MONOCHROME
             || params().stereo_mode() == MODE_RED_CYAN_HALF_COLOR
             && params().stereo_mode() != MODE_RED_CYAN_FULL_COLOR
             && params().stereo_mode() != MODE_RED_CYAN_DUBOIS
             && params().stereo_mode() != MODE_GREEN_MAGENTA_MONOCHROME
             && params().stereo_mode() != MODE_GREEN_MAGENTA_HALF_COLOR
             && params().stereo_mode() != MODE_GREEN_MAGENTA_FULL_COLOR
             && params().stereo_mode() != MODE_GREEN_MAGENTA_DUBOIS
             && params().stereo_mode() != MODE_AMBER_BLUE_MONOCHROME
             && params().stereo_mode() != MODE_AMBER_BLUE_HALF_COLOR
             && params().stereo_mode() != MODE_AMBER_BLUE_FULL_COLOR
             && params().stereo_mode() != MODE_AMBER_BLUE_DUBOIS
             && params().stereo_mode() != MODE_RED_BLUE_MONOCHROME
             && params().stereo_mode() != MODE_RED_CYAN_MONOCHROME)
    {
        draw_quad(data.rect, my_tex_coords);
    }
    else if ((params().stereo_mode() == MODE_MONO_LEFT && !data.mono_right_instead_of_left)
               || (params().stereo_mode() == MODE_ALTERNATING && data.display_frameno % 2 == 0))
    {
        glUniform1f(glGetUniformLocation(render_hardware::program_id (), "channel"), 0.0f);
        draw_quad(data.rect, my_tex_coords);
    }
    else if (params().stereo_mode() == MODE_MONO_RIGHT
               || (params().stereo_mode() == MODE_MONO_LEFT && data.mono_right_instead_of_left)
               || (params().stereo_mode() == MODE_ALTERNATING && data.display_frameno % 2 == 1))
    {
        glUniform1f(glGetUniformLocation(program_id (), "channel"), 1.0f);
        draw_quad(data.rect, my_tex_coords);
    }
    else if (params().stereo_mode() == MODE_MONO_LEFT
               || params().stereo_mode() == MODE_LEFT_RIGHT_HALF
               || params().stereo_mode() == MODE_TOP_BOTTOM
               || params().stereo_mode() == MODE_TOP_BOTTOM_HALF
               || params().stereo_mode() == MODE_HDMI_FRAME_PACK)
    {
        glUniform1f(glGetUniformLocation(program_id (), "channel"), 0.0f);
        draw_quad(data.rect, my_tex_coords);
        glViewport(data.viewport[1][0], data.viewport[1][1], data.viewport[1][2], data.viewport[1][3]);
        glUniform1f(glGetUniformLocation(program_id (), "channel"), 1.0f);
        draw_quad(data.rect, my_tex_coords);
    }
}
void render_hardware::render(const video_frame &frame, int _active_index,
                             color_hardware *color,
                             const display_data &data)
{
    int ppsy = (data.screen.y() + data.screen.height() - data.viewport[0][1] - data.viewport[0][3]) ;
    int ppsx = (data.screen.x() + data.viewport[0][0]) ;

    if (!program_id() || !is_compatible())
    {
        deinit();
        init();
    }
    last_params() = params();
    last_frame() = frame;

    int left = 0;
    int right = (frame.stereo_layout == STEREO_MONO ? 0 : 1);
    if (params().stereo_mode_swap())
    {
        qSwap(left, right);
    }

    if ((params().stereo_mode() == MODE_EVEN_ODD_ROWS
         || params().stereo_mode() == MODE_CHECKERBOARD)
            && ppsy %2 == 0)
    {
        qSwap(left, right);
    }
    if ((params().stereo_mode() == MODE_EVEN_ODD_COLUMNS
         || params().stereo_mode() == MODE_CHECKERBOARD)
            && ppsx%2== 1)
    {
        qSwap(left, right);
    }

    glEnable(GL_TEXTURE_2D);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

    // 应用全屏幕翻转/转弯
    float my_tex_coords[2][4][2];
    memcpy(my_tex_coords, data.tex_coords, sizeof(my_tex_coords));

    if (ctrl_core::parameter().fullscreen())
    {
        if (params().fullscreen_flip_left())
        {
            qSwap(my_tex_coords[0][0][0], my_tex_coords[0][3][0]);
            qSwap(my_tex_coords[0][0][1], my_tex_coords[0][3][1]);
            qSwap(my_tex_coords[0][1][0], my_tex_coords[0][2][0]);
            qSwap(my_tex_coords[0][1][1], my_tex_coords[0][2][1]);
        }
        if (params().fullscreen_flop_left())
        {
            qSwap(my_tex_coords[0][0][0], my_tex_coords[0][1][0]);
            qSwap(my_tex_coords[0][0][1], my_tex_coords[0][1][1]);
            qSwap(my_tex_coords[0][3][0], my_tex_coords[0][2][0]);
            qSwap(my_tex_coords[0][3][1], my_tex_coords[0][2][1]);
        }
        if (params().fullscreen_flip_right())
        {
            qSwap(my_tex_coords[1][0][0], my_tex_coords[1][3][0]);
            qSwap(my_tex_coords[1][0][1], my_tex_coords[1][3][1]);
            qSwap(my_tex_coords[1][1][0], my_tex_coords[1][2][0]);
            qSwap(my_tex_coords[1][1][1], my_tex_coords[1][2][1]);
        }
        if (params().fullscreen_flop_right())
        {
            qSwap(my_tex_coords[1][0][0], my_tex_coords[1][1][0]);
            qSwap(my_tex_coords[1][0][1], my_tex_coords[1][1][1]);
            qSwap(my_tex_coords[1][3][0], my_tex_coords[1][2][0]);
            qSwap(my_tex_coords[1][3][1], my_tex_coords[1][2][1]);
        }
    }

    glUseProgram(program_id ());
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, color->texture_id(_active_index, left));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, color->texture_id(_active_index, right));

    glUniform1i(glGetUniformLocation(program_id (), "rgb_l"), 0);
    glUniform1i(glGetUniformLocation(program_id (), "rgb_r"), 1);
    glUniform1f(glGetUniformLocation(program_id (), "parallax"),
                params().parallax() * 0.05f
                * (params().stereo_mode_swap() ? -1 : +1));


    crosstalk();
    mask_tex(data.viewport);

    draw_stereo(data, my_tex_coords);

    Q_ASSERT(check_error(HERE));
    glUseProgram(0);
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glDisable(GL_TEXTURE_2D);

    draw_window_pos(data);
}
void render_hardware::mask_tex(const GLint viewport[2][4])
{
    if (params().stereo_mode() == MODE_EVEN_ODD_ROWS
            || params().stereo_mode() == MODE_EVEN_ODD_COLUMNS
            || params().stereo_mode() == MODE_CHECKERBOARD)
    {
        glUniform1i(glGetUniformLocation(program_id (), "mask_tex"),
                    3);
        glUniform1f(glGetUniformLocation(program_id (), "step_x"),
                    1.0f / static_cast<float>(viewport[0][2]));
        glUniform1f(glGetUniformLocation(program_id (), "step_y"),
                    1.0f / static_cast<float>(viewport[0][3]));
    }
}
void render_hardware::color_adjust()
{
    if (needs_coloradjust(params()))
    {
        float color_matrix[16];
        get_color_matrix(params().brightness(), params().contrast(),
                         params().hue(), params().saturation(), color_matrix);
        glUniformMatrix4fv(glGetUniformLocation(program_id (), "color_matrix"),
                           1, GL_TRUE, color_matrix);
    }
}
void render_hardware::crosstalk()
{
    if (params().stereo_mode() != MODE_RED_GREEN_MONOCHROME
            && params().stereo_mode() != MODE_RED_CYAN_HALF_COLOR
            && params().stereo_mode() != MODE_RED_CYAN_FULL_COLOR
            && params().stereo_mode() != MODE_RED_CYAN_DUBOIS
            && params().stereo_mode() != MODE_GREEN_MAGENTA_MONOCHROME
            && params().stereo_mode() != MODE_GREEN_MAGENTA_HALF_COLOR
            && params().stereo_mode() != MODE_GREEN_MAGENTA_FULL_COLOR
            && params().stereo_mode() != MODE_GREEN_MAGENTA_DUBOIS
            && params().stereo_mode() != MODE_AMBER_BLUE_MONOCHROME
            && params().stereo_mode() != MODE_AMBER_BLUE_HALF_COLOR
            && params().stereo_mode() != MODE_AMBER_BLUE_FULL_COLOR
            && params().stereo_mode() != MODE_AMBER_BLUE_DUBOIS
            && params().stereo_mode() != MODE_RED_BLUE_MONOCHROME
            && params().stereo_mode() != MODE_RED_CYAN_MONOCHROME
            && render_hardware::needs_ghostbust(params()))
    {
        glUniform3f(glGetUniformLocation(program_id (), "crosstalk"),
                    params().crosstalk_r() * params().ghostbust(),
                    params().crosstalk_g() * params().ghostbust(),
                    params().crosstalk_b() * params().ghostbust());
    }
}

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

