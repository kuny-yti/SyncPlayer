#ifndef RENDER_HARDWARE_H
#define RENDER_HARDWARE_H
#include "hardware.h"
#include "parameters.h"
#include "video_frame.h"
#include "blob.h"
#include "color_hardware.h"

struct display_data
{
    qint64 display_frameno;
    bool keep_viewport;
    bool mono_right_instead_of_left;
    QRectF rect;//绘制矩形
    QRectF screen;
    GLint viewport[2][4];
    float tex_coords[2][4][2];
    QSize dst_size;
    StereoMode stereo_mode;
};

class render_hardware:public hardware
{
    QString fs_src;

    parameters _params;          // 显示当前参数
    parameters _last_params;     // 在这一步中最后一个参数用于初始化检查
    video_frame _last_frame;     // 这个步骤的最后一帧用于初始化检查
    uint _prg;                 // 读取sRGB纹理，根据_params [ _active_index ]
    uint _dummy_tex;           // 一个空的字幕的纹理
    uint _mask_tex;            // 为掩蔽模式奇偶- { }的行，列，棋盘
    blob _3d_ready_sync_buf;            // 三维准备同步像素

    void draw_window_pos(const display_data &data);
    void crosstalk();
    void color_adjust();
    void mask_tex(const GLint viewport[2][4]);
    void draw_stereo(const display_data &data, const float my_tex_coords[2][4][2]);

public:
    render_hardware();

    blob & ready_sync_3d() {return _3d_ready_sync_buf;}
    uint program_id() {return _prg;}
    QString &fragment() { return fs_src;}
    parameters &params(){return _params;}
    parameters &last_params(){return _last_params;}
    video_frame &last_frame(){return _last_frame;}

    void init();
    void deinit();
    bool needs_subtitle(const parameters& params);
    bool needs_coloradjust(const parameters& params);
    bool needs_ghostbust(const parameters& params);
    bool is_compatible();

    void render(const video_frame &frame, int _active_index, color_hardware *color,
                const display_data &data);

    static void draw_quad(const QRectF &rect,
                const float tex_coords[2][4][2] = NULL,
                const float more_tex_coords[4][2] = NULL) ;
};

#endif // RENDER_HARDWARE_H
