#ifndef RENDER_UPDATE_H
#define RENDER_UPDATE_H

#include "thread_base.h"
#include "video_frame.h"
#include "subtitle_box.h"
#include "video_output_update.h"

class render_update : public thread
{
    Q_OBJECT
private:
    bool _render;
    int _w, _h;
    mutex _wait_mutex;
    condition _wait_cond;
    bool _action_activate;
    bool _action_prepare;
    bool _action_finished;
    bool _redisplay;
    video_frame _next_frame;
    subtitle_box _next_subtitle;
    bool _failure;
    // The display frame number
    qint64 _display_frameno;

    video_output_update* _vo_update;

public:
    render_update(video_output_update*);

    void set_render(bool r);
    void resize(int w, int h);
    void activate_next_frame();
    void prepare_next_frame(const video_frame &frame, const subtitle_box &subtitle);
    void redisplay();

    qint64 time_to_next_frame_presentation();

    void run();

    bool failure() const
    {
        return _failure;
    }
};
#endif // RENDER_UPDATE_H
