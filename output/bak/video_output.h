#ifndef VIDEO_OUTPUT_H
#define VIDEO_OUTPUT_H

#include "blob.h"
#include "video_frame.h"
#include "subtitle_box.h"
#include "controller.h"
//#include "subtitle_renderer.h"
#include <QOpenGLFunctions>
//#include "subtitle_updater.h"
#include "player.h"
#include "input_hardware.h"
#include "color_hardware.h"
#include "render_hardware.h"

class video_output :
        public input_hardware,
        public color_hardware,
        public render_hardware,
        public controller
{
private:
    bool _initialized;

    int _active_index;                  // 0 or 1
    video_frame _frame[2];              // 输入参数 (主 / 备)

    // 绘制两个视图视频帧的OpenGL视口坐标
    int _full_viewport[4];
    int _viewport[2][4];
    float _tex_coords[2][4][2];

private:

    void draw_quad(float x, float y, float w, float h,
                const float tex_coords[2][4][2] = NULL,
                const float more_tex_coords[4][2] = NULL) ;
protected:
    //subtitle_renderer _subtitle_renderer;

    // 获得整个视口大小。
    int full_display_width() const;
    int full_display_height() const;

    //得到的视口区域的大小是用于视频。这是可重写的均衡器。
    virtual int video_display_width() const;
    virtual int video_display_height() const;

    virtual bool context_is_stereo() const = 0; // 当前OpenGL上下文是否支持立体显示
    virtual void recreate_context(bool stereo) = 0;     // Recreate an OpenGL context and make it current
    virtual void trigger_resize(int w, int h) = 0;      //触发调整视频区

    void clear();                         // 清楚视频区域
    void reshape(const QSize &size, const parameters& params =
            ctrl_core::parameter());//调用这个视频时的区域的大小

    virtual float screen_pixel_aspect_ratio() const = 0;// 在屏幕上一个像素宽高比

    /* Get current video area properties */
    virtual int width() const = 0;              // in pixels
    virtual int height() const = 0;             // in pixels
    virtual int pos_x() const = 0;              // in pixels
    virtual int pos_y() const = 0;              // in pixels

    //显示当前帧。
    //1.用均衡器，需要设置一些特殊的性质。
    //2.NVIDIA SDI输出。
    //未完成：此处要处理交错帧
    void display_current_frame(display_data &data);

    void display_current_frame(qint64 display_frameno, int dst_width, int dst_height,
                               StereoMode stereo_mode);
    void display_current_frame(qint64 display_frameno = 0);

public:
    video_output();
    virtual ~video_output();
    /*初始化视频输出，或抛出一个异常*/
    virtual void init();
    /* 等待的字幕渲染初始化完成 ,如果有字幕视频需要等待数微秒 */
    virtual qint64 wait_for_subtitle_renderer() = 0;
    /* 取消初始设置视频输出 */
    virtual void deinit();

    /* 获取能力 */
    virtual bool supports_stereo() const = 0;           // OpenGL立体模式是否可用

    /* 准备一个新的帧。*/
    virtual void prepare_next_frame(const video_frame &frame);
    /* 切换到下一帧（使它当前的） */
    virtual void activate_next_frame();
    /* 获取估计下一帧时间，将显示在屏幕上 */
    virtual qint64 time_to_next_frame_presentation() const;
};

#endif // VIDEO_OUTPUT_H
