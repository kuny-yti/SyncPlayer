#ifndef VIDEO_OUTPUT_UPDATE_H
#define VIDEO_OUTPUT_UPDATE_H
#include "video_output.h"
#include <QRect>
#include <QOpenGLFramebufferObject>

class video_output_update :public video_output
{
private:
    int _screen_width, _screen_height;
    float _screen_pixel_aspect_ratio;
    bool _container_is_external;
    bool _fullscreen;
    QRect _geom;
    bool _screensaver_inhibited;
    bool _recreate_context;
    bool _recreate_context_stereo;
    QOpenGLFramebufferObject *fbo;
protected:
    virtual bool context_is_stereo() const;
    virtual void recreate_context(bool stereo);
    virtual void trigger_resize(int w, int h);

    virtual int screen_width() const;
    virtual int screen_height() const;
    virtual float screen_pixel_aspect_ratio() const;
    virtual int width() const;
    virtual int height() const;
    virtual int pos_x() const;
    virtual int pos_y() const;
public:
    video_output_update();
    virtual ~video_output_update();

    virtual void init();
    virtual qint64 wait_for_subtitle_renderer();
    virtual void deinit();

    virtual bool supports_stereo() const;

    virtual void prepare_next_frame(const video_frame &frame, const subtitle_box &subtitle);
    virtual void activate_next_frame();
    virtual qint64 time_to_next_frame_presentation() const;

    virtual void receive_command(const CommandType& cmd);

    friend class render_update;
};

#endif // VIDEO_OUTPUT_UPDATE_H
