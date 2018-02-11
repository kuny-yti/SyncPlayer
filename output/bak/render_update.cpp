#include "render_update.h"

render_update::render_update(video_output_update* vo_update):
    _render(false),
    _action_activate(false),
    _action_prepare(false),
    _action_finished(false),
    _failure(false),
    _display_frameno(0),
    _vo_update(vo_update)
{
}
void render_update::set_render(bool r)
{
    _redisplay = r;
    _render = r;
}

void render_update::resize(int w, int h)
{
    _w = w;
    _h = h;
}
void render_update::activate_next_frame()
{
    if (_failure)
        return;
    _wait_mutex.lock();
    _action_finished = false;
    _action_activate = true;
    while (_action_activate)
        _wait_cond.wait(&_wait_mutex);
    _action_finished = true;
    _wait_mutex.unlock();
}

void render_update::prepare_next_frame(const video_frame &frame, const subtitle_box &subtitle)
{
    if (_failure)
        return;
    _wait_mutex.lock();
    _next_subtitle = subtitle;
    _next_frame = frame;
    _action_finished = false;
    _action_prepare = true;
    while (_action_prepare)
        _wait_cond.wait(&_wait_mutex);
    _action_finished = true;
    _wait_mutex.unlock();
}
void render_update::redisplay()
{
    _redisplay = true;
}
void render_update::run()
{
    try {
        //Q_ASSERT(_vo_update_widget->context()->isValid());
        //_vo_update_widget->makeCurrent();
        //assert(QGLContext::currentContext() == _vo_update_widget->context());
        while (_render)
        {
#ifdef Q_WS_X11
            GLuint counter;
            if (GLXEW_SGI_video_sync && glXGetVideoSyncSGI(&counter) == 0)
                _display_frameno = counter;
            else
                _display_frameno++;
#else
            _display_frameno++;
#endif
            // In alternating mode, we should always present both left and right view of
            // a stereo video frame before advancing to the next video frame. This means we
            // can only switch video frames every other output frame.
            if (ctrl_core::parameter().stereo_mode() != MODE_ALTERNATING
                    || _display_frameno % 2 == 0)
            {
                _wait_mutex.lock();
                if (_action_activate)
                {
                    try {
                        _vo_update->video_output::activate_next_frame();
                    }
                    catch (QString& e)
                    {
                        //_e = e;
                        _render = false;
                        _failure = true;
                    }
                    _action_activate = false;
                    _wait_cond.wakeOne();
                    _redisplay = true;
                }
                _wait_mutex.unlock();
            }
            if (_failure)
                break;
            _wait_mutex.lock();
            if (_action_prepare)
            {
                try
                {
                    _vo_update->video_output::prepare_next_frame(_next_frame);
                }
                catch (QString& e)
                {
                    //_e = e;
                    _render = false;
                    _failure = true;
                }
                _action_prepare = false;
                _wait_cond.wakeOne();
            }
            _wait_mutex.unlock();
            if (_failure)
                break;
            if (_w > 0 && _h > 0
                    && (_vo_update->full_display_width() != _w
                        || _vo_update->full_display_height() != _h))
            {
                _vo_update->reshape(QSize(_w, _h));
                _redisplay = true;
            }
            // In alternating mode, we always need to redisplay
            if (ctrl_core::parameter().stereo_mode() == MODE_ALTERNATING)
                _redisplay = true;
            // If DLP 3-D Ready Sync is active, we always need to redisplay
            if (ctrl_core::parameter().fullscreen() && ctrl_core::parameter().fullscreen_3d_ready_sync()
                    && (ctrl_core::parameter().stereo_mode() == MODE_LEFT_RIGHT
                        || ctrl_core::parameter().stereo_mode() == MODE_LEFT_RIGHT_HALF
                        || ctrl_core::parameter().stereo_mode() == MODE_TOP_BOTTOM
                        || ctrl_core::parameter().stereo_mode() == MODE_TOP_BOTTOM_HALF
                        || ctrl_core::parameter().stereo_mode() == MODE_ALTERNATING))
                _redisplay = true;
            // Redisplay if necessary
            if (_redisplay)
            {
                _redisplay = false;
#if HAVE_LIBXNVCTRL
                _vo_update->sdi_output(_display_frameno);
#endif // HAVE_LIBXNVCTRL
                _vo_update->display_current_frame(_display_frameno);
                // Swap buffers. To avoid problems with the NVIDIA 304.x driver
                // series on GNU/Linux, we first release the GL context and re-grab
                // it after the swap. Not sure why that would be necessary, but it
                // fixes a problem where swapBuffers() does not return and the
                // application blocks.
                // However, this breaks video playback on Windows, so only do it on
                // X11 systems.

            } else if (!ctrl_core::parameter().benchmark())
            {
                // do not busy loop
                usleep(1000);
            }
        }
    }
    catch (QString& e)
    {
        //_e = e;
        _render = false;
        _failure = true;
    }
    _wait_mutex.lock();
    if (_action_activate || _action_prepare)
    {
        while (!_action_finished)
        {
            _wait_cond.wakeOne();
            _wait_mutex.unlock();
            _wait_mutex.lock();
        }
    }
    _wait_mutex.unlock();
}

qint64 render_update::time_to_next_frame_presentation()
{
    // TODO: return a good estimate of the number of microseconds that will
    // pass until the next buffer swap completes. For now, just assume that
    // the next frame will display immediately.
    return 0;
}
