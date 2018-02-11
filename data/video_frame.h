#ifndef VIDEO_FRAME_H
#define VIDEO_FRAME_H

#include <QString>
#include <QSize>
#include "type.h"

class video_frame
{
public:
    QSize raw_size;                 // 像素数据
    float raw_aspect_ratio;         // 数据的纵横比
    QSize size;                     // 视图像素大小
    float aspect_ratio;             // 视图纵横比
    DataLayout layout;              // 数据布局
    ColorSpace color_space;         // 颜色空间
    ValueRange value_range;         // 值空间
    ChromaLocation chroma_location; // 色度取样位置
    StereoLayout stereo_layout;     // 立体布局
    bool stereo_layout_swap;        // 立体布局是否交换左右视图

    void *data[2][3];               // 1-3 平面数据 ， 1-2视力. 如果未使用为空。
    size_t line_size[2][3];         // Line size for 1-3 planes in 1-2 views. 0 if unused.

    qint64 p_time;       // 演示时间戳

    video_frame();

    // 设置视图尺寸
    void set_view_dimensions();

    // 是否包含有效数据
    bool is_valid() const
    {
        return !raw_size.isEmpty();
    }

    // 拷贝数据到指定的视图中(0=left, 1=right) 和指定面中
    void copy_plane(int view, int plane, void *dst) const;

    // 返回一个描述格式字符串（布局，颜色空间，色差值范围，位置）
    QString format_info() const;    // Human readable information
    QString format_name() const;    // Short code
};

#endif // VIDEO_FRAME_H
