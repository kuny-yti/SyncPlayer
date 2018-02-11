#ifndef AUDIO_OUTPUT_H
#define AUDIO_OUTPUT_H
extern "C"
{
    #if !defined(__APPLE__) || defined(HAVE_AL_AL_H)
    #  include <AL/al.h>
    #  include <AL/alc.h>
    #  include <AL/alext.h>
    #else
    #  include <OpenAL/al.h>
    #  include <OpenAL/alc.h>
    #  include <OpenAL/alext.h>
    #endif
}
#include "audio_blob.h"

class audio_output
{
private:
    // 静态配置
    static const size_t _num_buffers;   // 音频缓冲区数
    static const size_t _buffer_size;   // 每个音频缓冲区的大小

    // OpenAL things
    QVector<QString> _devices;          // 已知的OpenAL的设备列表
    bool _initialized;                  // 是否初始化
    ALCdevice *_device;                 // 音频设备
    ALCcontext *_context;               // 音频设备上下文
    QVector<ALuint> _buffers;           // 缓冲区句柄
    ALuint _source;                     // 音频源
    ALint _state;                       // 音频源状态

    // 当前音频数据缓冲的性质
    QVector<qint64> _buffer_channels;      // 通道数
    QVector<qint64> _buffer_sample_bits;   // 比特率
    QVector<qint64> _buffer_rates;         // 采样率 (Hz)

    // 时间管理
    qint64 _past_time;                 // 过去的时间
    qint64 _last_time;                 //最后时间戳
    qint64 _ext_last_time;             //最后时间戳转定时器
    qint64 _last_reported;             //上次报告的时间戳

    // 得到一个在blob音频数据源格式的OpenAL（或抛出一个异常）
    ALenum al_format(const audio_blob &blob);


public:
    audio_output();
    ~audio_output();

    // 设置源参数
    void set_volume(float val);
    /* 可用的OpenAL设备数量*/
    int devices() const;
    /* 返回指定OpenAL设备的名称 */
    const QString &device_name(int i) const;

    /* 初始化音频输出设备，如果 i < 0使用默认设备。初始化失败抛出异常*/
    void init(int i = -1);
    /* 取消初始设置音频设备。 */
    void deinit();

    //所需的初始数据大小
    size_t initial_size() const;
    //所需的更新数据大小
    size_t update_size() const;

    //状态
    qint64 status(bool *need_data);
    //向OpenAL写入数据
    void data(const audio_blob &blob);

    //启动音频输出
    qint64 start();

    //暂停音频输出
    void pause();
    //取消暂停
    void unpause();

    //停止音频输出
    void stop();
};

#endif // AUDIO_OUTPUT_H
