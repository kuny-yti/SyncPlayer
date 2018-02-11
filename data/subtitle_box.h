#ifndef SUBTITLE_BOX_H
#define SUBTITLE_BOX_H
#include "type.h"
#include <QVector>
#include <QDataStream>
#include <QTextStream>
#include <QSize>
#include <QPoint>

class subtitle_box
{
public:
    // 图像数据
    class image_t
    {
    public:
        QSize size;                // 尺寸
        QPoint pos;                // 关于视频帧位置
        QVector<quint8> palette;   // 调色板, 有 RGBA组成一组调色板
        QVector<quint8> data;      // 使用位图调色板
        size_t linesize;           // 位图的线尺寸

        void save(QDataStream &os) const;
        void load(QDataStream &is);
    };

    QString language;               // 语言信息 (空为未知)

    // Data
    SubtitleFormat format;          // 字幕数据格式
    QString style;                  // 样式信息 (ass格式为只读)
    QString str;                    // 时间的文本 (只有ass格式或text格式)
    QVector<image_t> images;        // 图像。需要alpha混合。

    // 显示时间信息
    qint64 p_start_time;            // 演示时间戳
    qint64 p_stop_time;             // 演示结束时间戳

    subtitle_box();

    bool operator==(const subtitle_box &box) const
    {
        if (!is_valid() && !box.is_valid())
        {
            return true;
        }
        else if (format == FORMAT_IMAGE)
        {
            return (p_start_time == box.p_start_time
                    && p_stop_time == box.p_stop_time);
        }
        else
        {
            return style == box.style && str == box.str;
        }
    }
    bool operator!=(const subtitle_box &box) const
    {
        return !operator==(box);
    }

    // 是否包含有效数据
    bool is_valid() const
    {
        return (((format == FORMAT_ASS || format == FORMAT_TEXT) && !str.isEmpty())
                || (format == FORMAT_IMAGE && !images.empty()));
    }

    // 在完整呈现期间保持不变
    // （ ASS 字幕可以是动画，因此需要渲染当时钟的变化）
    bool is_constant() const
    {
        return (format != FORMAT_ASS);
    }

    // 返回一个描述格式字符串
    QString format_info() const;    // Human readable information
    QString format_name() const;    // Short code

    // 序列化
    void save(QDataStream &os) const;
    void load(QDataStream &is);
};

#endif // SUBTITLE_BOX_H
