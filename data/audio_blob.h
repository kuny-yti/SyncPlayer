#ifndef AUDIO_BLOB_H
#define AUDIO_BLOB_H
#include "type.h"
#include <QString>

class audio_blob
{
public:
    QString language;             // 语言信息(若为空则未知)

    int channels;                 // 声道数:1 (单声道)、 2 (立体声)、 4 (四声道), 6 (5:1), 7 (6:1), or 8 (7:1)
    int rate;                     // 采样率
    SampleFormat sample_format;   // 采样格式

    void *data;                   // 数据指针
    size_t size;                  // 数据大小

    qint64 p_time;                // 演示时间戳

    audio_blob();

    // 是否包含有效数据
    bool is_valid() const
    {
        return (channels > 0 && rate > 0);
    }

    // 返回一个描述格式字符串
    QString format_info() const;    //
    QString format_name() const;    //

    // 返回采样格式比特率
    int sample_bits() const;
};

#endif // AUDIO_BLOB_H
