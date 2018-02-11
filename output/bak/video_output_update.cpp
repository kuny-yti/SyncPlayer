#include "video_output_update.h"
#include <QDesktopWidget>
#include <QApplication>
#include <cmath>

video_output_update::video_output_update() :
    video_output(),
    _container_is_external(false),
    _fullscreen(false),
    _screensaver_inhibited(false),
    _recreate_context(false),
    _recreate_context_stereo(false)
{
    _screen_width = QApplication::desktop()->screenGeometry().width();
    _screen_height = QApplication::desktop()->screenGeometry().height();
    _screen_pixel_aspect_ratio =
            static_cast<float>(_screen_height)
            / static_cast<float>(_screen_width);
    if (fabs(_screen_pixel_aspect_ratio - 1.0f) < 0.03f)
    {
        _screen_pixel_aspect_ratio = 1.0f;
    }
}
video_output_update::~video_output_update()
{

}
void video_output_update::init()
{
    //fbo = new QOpenGLFramebufferObject();
    video_output::init();
    video_output::clear();
    // Initialize GL things
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
}
qint64 video_output_update::wait_for_subtitle_renderer()
{
    /*if (_subtitle_renderer.is_initialized())
    {
        return 0;
    }*/
    qint64 wait_start = get_microseconds(TIMER_MONOTONIC);
    QString init_exception;
    {
        //msg::wrn(_("Waiting for subtitle renderer initialization..."));
    }
    try
    {
        //while (!_subtitle_renderer.is_initialized())
        {
            //process_events();
            //usleep(10000);
        }
    }
    catch (QString &e)
    {
        init_exception = e;
    }
    if (!init_exception.isEmpty())
    {
        throw init_exception;
    }
    qint64 wait_stop = get_microseconds(TIMER_MONOTONIC);
    return (wait_stop - wait_start);
}
void video_output_update::deinit()
{
    try
    {
        video_output::deinit();
    }
    catch (QString& e)
    {
        //QMessageBox::critical(_widget, _("Error"), e.what());
    }
}
bool video_output_update::context_is_stereo() const
{
    //return (_format.stereo());
}

void video_output_update::recreate_context(bool stereo)
{
    _recreate_context = true;
    _recreate_context_stereo = stereo;
}
void video_output_update::trigger_resize(int w, int h)
{
}
bool video_output_update::supports_stereo() const
{
    /*QGLFormat fmt = _format;
    fmt.setStereo(true);
    QGLWidget *tmpwidget = new QGLWidget(fmt);
    bool ret = tmpwidget->format().stereo();
    delete tmpwidget;
    return ret;*/
}
int video_output_update::screen_width() const
{
    return _screen_width;
}

int video_output_update::screen_height() const
{
    return _screen_height;
}

float video_output_update::screen_pixel_aspect_ratio() const
{
    return _screen_pixel_aspect_ratio;
}

int video_output_update::width() const
{
    //return _widget->vo_width();
}

int video_output_update::height() const
{
    //return _widget->vo_height();
}

int video_output_update::pos_x() const
{
    //return _widget->vo_pos_x();
}

int video_output_update::pos_y() const
{
    //return _widget->vo_pos_y();
}

void video_output_update::prepare_next_frame(const video_frame &frame, const subtitle_box &subtitle)
{
    //if (_widget)
     //   _widget->gl_thread()->prepare_next_frame(frame, subtitle);
}

void video_output_update::activate_next_frame()
{
    //if (_widget)
    //    _widget->gl_thread()->activate_next_frame();
}
qint64 video_output_update::time_to_next_frame_presentation() const
{
    //return (_widget ? _widget->gl_thread()->time_to_next_frame_presentation() : 0);
}
void video_output_update::receive_command(const CommandType& cmd)
{
    if ((cmd == CMD_QUALITY
            || cmd == CMD_STEREO_MODE
            || cmd == CMD_STEREO_MODE_SWAP
            || cmd == CMD_CONTRAST
            || cmd == CMD_BRIGHTNESS
            || cmd == CMD_HUE
            || cmd == CMD_SATURATION
            || cmd == CMD_PARALLAX
            || cmd == CMD_GHOSTBUST))
    {
            //更新一次渲染数据
    }

}
