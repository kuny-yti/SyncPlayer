#ifndef COLOR_HARDWARE_H
#define COLOR_HARDWARE_H
#include "hardware.h"
#include "video_frame.h"
#include "parameters.h"
#include "input_hardware.h"

class color_hardware :public hardware
{
    QString fs_src;
    parameters _last_params[2];   // 这个步骤的最后一个参数用于初始化检查
    video_frame _last_frame[2];   // 这个步骤的最后一帧用于初始化检查
    uint _prg[2];               // 颜色空间转换，色彩调整
    uint _fbo;                  // 帧缓冲对象渲染到纹理的sRGB
    uint _tex[2][2];            // 输出：srgb8或线性rgb16纹理

    int _yuv_chroma_width_divisor;  // 对于YUV格式：色度抽样
    int _yuv_chroma_height_divisor; // 对于YUV格式：色度抽样
public:
    color_hardware();

    uint &fbo_id() {return _fbo;}
    uint &program_id(int ack) {return _prg[ack];}
    uint &texture_id(int r, int c) {return _tex[r][c];}
    parameters &last_params(int ack){return _last_params[ack];}
    video_frame &last_frame(int ack){return _last_frame[ack];}
    QString &fragment() { return fs_src;}

    void update_chroma(int w, int h)
    {
        _yuv_chroma_width_divisor = w;
        _yuv_chroma_height_divisor = h;
    }
    void init(int index, const parameters& params, const video_frame &frame);
    void deinit(int index);
    bool is_compatible(int index, const parameters& params,
                       const video_frame &current_frame);

    void correction(int index, const video_frame &frame, input_hardware * input);
};

#endif // COLOR_HARDWARE_H
