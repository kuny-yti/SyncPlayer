#ifndef INPUT_HARDWARE_H
#define INPUT_HARDWARE_H

#include "video_frame.h"
#include "hardware.h"

class input_hardware :public hardware
{
    video_frame _last_frame;      // 最后一帧
    uint _pbo;                    // 纹理上传像素缓冲区对象
    uint _fbo;                    // 帧缓冲对象纹理清除
    uint _yuv_y_tex[2];           // 对于YUV格式：Y分量
    uint _yuv_u_tex[2];           // 对于YUV格式：U分量
    uint _yuv_v_tex[2];           // 对于YUV格式：V分量
    uint _bgra32_tex[2];          // fbgra32 格式
    int _yuv_chroma_width_divisor;  // 对于YUV格式：色度抽样
    int _yuv_chroma_height_divisor; // 对于YUV格式：色度抽样
public:
    input_hardware();

    uint &pbo_id(){return _pbo;}
    uint &fbo_id(){return _fbo;}
    uint &bgra32_texture(int ack){return _input_bgra32_tex[ack];}
    uint &yuv_v_texture(int ack){return _yuv_v_tex[ack];}
    uint &yuv_u_texture(int ack){return _yuv_u_tex[ack];}
    uint &yuv_y_texture(int ack){return _yuv_y_tex[ack];}
    int &yuv_chroma_width() {return _yuv_chroma_width_divisor;}
    int &yuv_chroma_height() {return _yuv_chroma_height_divisor;}

    video_frame &last_frame() {return _last_frame;}
    void upload_frame(const video_frame &frame);

    void init(const video_frame &frame);
    void deinit();
    bool is_compatible(const video_frame &current_frame);
};

#endif // INPUT_HARDWARE_H
