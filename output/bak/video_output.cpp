#include "video_output.h"
#include <cmath>

static const float full_tex_coords[2][4][2] =
{
    { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } },
    { { 0.0f, 0.0f }, { 1.0f, 0.0f }, { 1.0f, 1.0f }, { 0.0f, 1.0f } }
};
static void compute_viewport_and_tex_coords(int vp[4], float tc[4][2],
        float src_ar, int w, int h, float dst_w, float dst_h, float dst_ar,
        float crop_ar, float zoom, bool need_even_width, bool need_even_height)
{
    memcpy(tc, full_tex_coords[0], 4 * 2 * sizeof(float));
    if (crop_ar > 0.0f)
    {
        if (src_ar >= crop_ar)
        {
            float cutoff = (1.0f - crop_ar / src_ar) / 2.0f;
            tc[0][0] += cutoff;
            tc[1][0] -= cutoff;
            tc[2][0] -= cutoff;
            tc[3][0] += cutoff;
        }
        else
        {
            float cutoff = (1.0f - src_ar / crop_ar) / 2.0f;
            tc[0][1] += cutoff;
            tc[1][1] += cutoff;
            tc[2][1] -= cutoff;
            tc[3][1] -= cutoff;
        }
        src_ar = crop_ar;
    }
    if (src_ar >= dst_ar)
    {
        // need black borders top and bottom
        float zoom_src_ar = zoom * dst_ar + (1.0f - zoom) * src_ar;
        vp[2] = dst_w;
        vp[3] = dst_ar / zoom_src_ar * dst_h;
        float cutoff = (1.0f - zoom_src_ar / src_ar) / 2.0f;
        tc[0][0] += cutoff;
        tc[1][0] -= cutoff;
        tc[2][0] -= cutoff;
        tc[3][0] += cutoff;
    }
    else
    {
        // need black borders left and right
        vp[2] = src_ar / dst_ar * dst_w;
        vp[3] = dst_h;
    }
    if (need_even_width && vp[2] > 1 && vp[2] % 2 == 1)
        vp[2]--;
    if (need_even_height && vp[3] > 1 && vp[3] % 2 == 1)
        vp[3]--;
    vp[0] = (w - vp[2]) / 2;
    vp[1] = (h - vp[3]) / 2;
}

video_output::video_output():
    _initialized(false)
{
    _active_index = 1;
}

video_output::~video_output()
{
}

void video_output::init()
{
    if (!_initialized)
    {
        _full_viewport[0] = -1;
        _full_viewport[1] = -1;
        _full_viewport[2] = -1;
        _full_viewport[3] = -1;
        _initialized = true;
    }
}
void video_output::deinit()
{
    if (_initialized)
    {
        clear();
        check_error(HERE);

        input_hardware::deinit();
        color_hardware::deinit(0);
        color_hardware::deinit(1);
        render_hardware::deinit();
        check_error(HERE);
        _initialized = false;
    }
}

void video_output::clear()
{
    Q_ASSERT(check_error(HERE));
    if (context_is_stereo())
    {
        glDrawBuffer(GL_BACK_LEFT);
        glClear(GL_COLOR_BUFFER_BIT);
        glDrawBuffer(GL_BACK_RIGHT);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    else
    {
        glClear(GL_COLOR_BUFFER_BIT);
    }
    Q_ASSERT(check_error(HERE));
}

int video_output::full_display_width() const
{
    return _full_viewport[2];
}

int video_output::full_display_height() const
{
    return _full_viewport[3];
}

int video_output::video_display_width() const
{
    Q_ASSERT(_viewport[0][2] > 0);
    return _viewport[0][2];
}
int video_output::video_display_height() const
{
    Q_ASSERT(_viewport[0][3] > 0);
    return _viewport[0][3];
}
void video_output::reshape(const QSize &size, const parameters& params)
{
    int w = size.width ();
    int h = size.height ();
    // Clear
    _full_viewport[0] = 0;
    _full_viewport[1] = 0;
    _full_viewport[2] = w;
    _full_viewport[3] = h;
    glViewport(0, 0, w, h);
    clear();
    _viewport[0][0] = 0;
    _viewport[0][1] = 0;
    _viewport[0][2] = w;
    _viewport[0][3] = h;
    _viewport[1][0] = 0;
    _viewport[1][1] = 0;
    _viewport[1][2] = w;
    _viewport[1][3] = h;
    memcpy(_tex_coords, full_tex_coords, sizeof(_tex_coords));
    if (!_frame[_active_index].is_valid())
        return;

    bool need_even_width = (params.stereo_mode() == MODE_EVEN_ODD_COLUMNS
            || params.stereo_mode() == MODE_CHECKERBOARD);
    bool need_even_height = (params.stereo_mode() == MODE_EVEN_ODD_ROWS
            || params.stereo_mode() == MODE_CHECKERBOARD);

    // Compute viewport with the right aspect ratio
    if (params.stereo_mode() == MODE_LEFT_RIGHT
            || params.stereo_mode() == MODE_LEFT_RIGHT_HALF)
    {
        float dst_w = w / 2;
        float dst_h = h;
        float dst_ar = dst_w * screen_pixel_aspect_ratio() / dst_h;
        float src_ar = _frame[_active_index].aspect_ratio;
        float crop_ar = params.crop_aspect_ratio();
        if (params.stereo_mode() == MODE_LEFT_RIGHT_HALF)
        {
            src_ar /= 2.0f;
            crop_ar /= 2.0f;
        }
        compute_viewport_and_tex_coords(_viewport[0], _tex_coords[0], src_ar,
                w / 2, h, dst_w, dst_h, dst_ar,
                crop_ar, params.zoom(), need_even_width, need_even_height);
        memcpy(_viewport[1], _viewport[0], sizeof(_viewport[1]));
        _viewport[1][0] = _viewport[0][0] + w / 2;
        memcpy(_tex_coords[1], _tex_coords[0], sizeof(_tex_coords[1]));
    }
    else if (params.stereo_mode() == MODE_TOP_BOTTOM
            || params.stereo_mode() == MODE_TOP_BOTTOM_HALF)
    {
        float dst_w = w;
        float dst_h = h / 2;
        float dst_ar = dst_w * screen_pixel_aspect_ratio() / dst_h;
        float src_ar = _frame[_active_index].aspect_ratio;
        float crop_ar = params.crop_aspect_ratio();
        if (params.stereo_mode() == MODE_TOP_BOTTOM_HALF)
        {
            src_ar *= 2.0f;
            crop_ar *= 2.0f;
        }
        compute_viewport_and_tex_coords(_viewport[0], _tex_coords[0], src_ar,
                w, h / 2, dst_w, dst_h, dst_ar,
                crop_ar, params.zoom(), need_even_width, need_even_height);
        memcpy(_viewport[1], _viewport[0], sizeof(_viewport[1]));
        _viewport[0][1] = _viewport[1][1] + h / 2;
        memcpy(_tex_coords[1], _tex_coords[0], sizeof(_tex_coords[1]));
    }
    else if (params.stereo_mode() == MODE_HDMI_FRAME_PACK)
    {
        int blank_lines = h / 49;
        float dst_w = w;
        float dst_h = (h - blank_lines) / 2;
        float dst_ar = dst_w * screen_pixel_aspect_ratio() / dst_h;
        float src_ar = _frame[_active_index].aspect_ratio;
        compute_viewport_and_tex_coords(_viewport[0], _tex_coords[0], src_ar,
                w, (h - blank_lines) / 2, dst_w, dst_h, dst_ar,
                params.crop_aspect_ratio(), params.zoom(), need_even_width, need_even_height);
        memcpy(_viewport[1], _viewport[0], sizeof(_viewport[1]));
        _viewport[0][1] = _viewport[1][1] + (h - blank_lines) / 2 + blank_lines;
        memcpy(_tex_coords[1], _tex_coords[0], sizeof(_tex_coords[1]));
    }
    else {
        float dst_w = w;
        float dst_h = h;
        float dst_ar = dst_w * screen_pixel_aspect_ratio() / dst_h;
        float src_ar = _frame[_active_index].aspect_ratio;
        compute_viewport_and_tex_coords(_viewport[0], _tex_coords[0], src_ar,
                w, h, dst_w, dst_h, dst_ar,
                params.crop_aspect_ratio(), params.zoom(), need_even_width, need_even_height);
        memcpy(_viewport[1], _viewport[0], sizeof(_viewport[1]));
        memcpy(_tex_coords[1], _tex_coords[0], sizeof(_tex_coords[1]));
    }
}

void video_output::display_current_frame(  display_data &data)
{
    data.screen.setX (pos_x());
    data.screen.setY (pos_y());
    data.screen.setWidth (width());
    data.screen.setHeight (height());
    clear();
    const video_frame &frame = _frame[_active_index];
    if (!frame.is_valid())
        return;

    render_hardware::params() = ctrl_core::parameter();
    render_hardware::params().set_quality(color_hardware::last_params(_active_index).quality());

    render_hardware::params().set_stereo_mode(data.stereo_mode);
    bool context_needs_stereo = (render_hardware::params().stereo_mode() == MODE_STEREO);
    if (context_needs_stereo != context_is_stereo())
    {
        recreate_context(context_needs_stereo);
        return;
    }

    if (!data.keep_viewport
            && (frame.size.width() != render_hardware::last_frame().size.width()
                || frame.size.height() != render_hardware::last_frame().size.height()
                || frame.aspect_ratio < render_hardware::last_frame().aspect_ratio
                || frame.aspect_ratio > render_hardware::last_frame().aspect_ratio
                || render_hardware::last_params().stereo_mode() != render_hardware::params ().stereo_mode()
                || render_hardware::last_params().crop_aspect_ratio() < render_hardware::params ().crop_aspect_ratio()
                || render_hardware::last_params().crop_aspect_ratio() > render_hardware::params ().crop_aspect_ratio()
                || render_hardware::last_params().zoom() < render_hardware::params ().zoom()
                || render_hardware::last_params().zoom() > render_hardware::params ().zoom()
                || full_display_width() != data.dst_size.width ()
                || full_display_height() != data.dst_size.height ()))
    {
        reshape(data.dst_size, render_hardware::params());
    }
    Q_ASSERT(check_error(HERE));

    render_hardware::render(frame, _active_index, this, data);
}

void video_output::display_current_frame(qint64 display_frameno, int dst_width, int dst_height,
                           StereoMode stereo_mode)
{
     display_data data;
     data.display_frameno = display_frameno;
     data.dst_size = QSize(dst_width, dst_height);
     data.stereo_mode = stereo_mode;
     data.keep_viewport = false;
     data.mono_right_instead_of_left = false;
     data.rect = QRectF(-1,-1,2,2);
     memcpy(&data.viewport, &_viewport, sizeof(_viewport));
     memcpy(&data.tex_coords, &_tex_coords, sizeof(_tex_coords));
     data.screen = QRectF(pos_x(),pos_y(), width(),height());

    display_current_frame(data);
}
void video_output::display_current_frame(qint64 display_frameno)
{
    display_data data;
    data.display_frameno = display_frameno;
    data.dst_size = QSize(full_display_width(), full_display_height());
    data.stereo_mode = ctrl_core::parameter().stereo_mode();
    data.keep_viewport = false;
    data.mono_right_instead_of_left = false;
    data.rect = QRectF(-1,-1,2,2);
    memcpy(&data.viewport, &_viewport, sizeof(_viewport));
    memcpy(&data.tex_coords, &_tex_coords, sizeof(_tex_coords));
    data.screen = QRectF(pos_x(),pos_y(), width(),height());

   display_current_frame(data);
}

static int next_multiple_of_4(int x)
{
    return (x / 4 + (x % 4 == 0 ? 0 : 1)) * 4;
}

void video_output::prepare_next_frame(const video_frame &frame)
{
    int index = (_active_index == 0 ? 1 : 0);
    if (!frame.is_valid())
    {
        _frame[index] = frame;
        return;
    }

    //上传帧数据
    Q_ASSERT(check_error(HERE));
    input_hardware::upload_frame(frame);
    _frame[index] = frame;
    Q_ASSERT(check_error(HERE));

    //颜色校正
    color_hardware::correction(index, frame, this);
}

void video_output::activate_next_frame()
{
    _active_index = (_active_index == 0 ? 1 : 0);
}

qint64 video_output::time_to_next_frame_presentation() const
{
    return 0;
}
