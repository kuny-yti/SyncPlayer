#ifndef MEDIA_INPUT_H
#define MEDIA_INPUT_H
#include "type.h"
#include "media_object.h"

class media_input_p;
class media_input
{
private:
    media_input_p *_dptr;

    void get_video_stream(int stream, int &object, int &object_video) const;
    void get_audio_stream(int stream, int &object, int &object_stream) const;
    void get_subtitle_stream(int stream, int &object, int &object_stream) const;
    void have_active();
public:
    media_input();
    ~media_input();

    /* 通过给定的URL的媒体对象打开输入。一个设备只能有一个url*/
    void open(const QStringList &urls, const device_request &dev_request = device_request());

    /* 获取信息 */
    // URL的数量（媒体对象的数目）
    int urls() const;
    // 获取给定索引的url
    const QString &url(int i) const;
    // 标识符。
    const QString &id() const;

    // Metadata
    bool is_device() const;
    int tags() const;
    const QString &tag_name(int i) const;
    const QString &tag_value(int i) const;
    const QString &tag_value(const QString &tag_name) const;

    // 输入视频流数。
    int video_streams() const;
    // 输入的音频流的数目。
    int audio_streams() const;
    // 输入字幕流数目。
    int subtitle_streams() const;

    // 获取给定视频流的名字。
    const QString &video_stream_name(int stream) const;
    // 获取给定音频流的名字。
    const QString &audio_stream_name(int stream) const;
    // 获取给定字幕流的名字。
    const QString &subtitle_stream_name(int stream) const;

    // 总持续时间。
    qint64 duration() const;

    // 关于活动的视频流信息，视频帧的所有属性。但不包含实际数据的形式。
    const video_frame &video_template() const;
    // 关于活动音频流信息
    const audio_blob &audio_template() const;
    // 关于活动字幕流信息
    const subtitle_box &subtitle_template() const;

    // 视频率信息。
    int video_rate_numerator() const;
    int video_rate_denominator() const;
    qint64 video_duration() const;       // 帧速率

    /*
     * 访问媒体数据
     */
    /* 设置活动的媒体流。 */
    int selected_video_stream() const;
    void select_video_stream(int stream);
    int selected_audio_stream() const;
    void select_audio_stream(int stream);
    int selected_subtitle_stream() const;
    void select_subtitle_stream(int stream);

    /* 检查是否有立体布局，输入支持。 */
    bool stereo_supported(StereoLayout layout, bool swap) const;
    /* 设置立体布局。必须由输入支持。 */
    void set_stereo_layout(StereoLayout layout, bool swap);

    /* 开始读取活动的流视频帧同步 。在一个单独的线程 */
    void start_video_read();
    /* 等待视频帧读取完成，并返回帧。 */
    video_frame finish_video_read();

    /* 开始读取给定数量的有源音频数据流，异步。 在一个单独的线程*/
    void start_audio_read(int size);
    /* 等待音频数据读取完毕，并返回数据。 */
    audio_blob finish_audio_read();

    /* 开始读取给定数量的字幕数据流，异步。 在一个单独的线程 */
    void start_subtitle_read();
    /*等待字幕数据读取完毕，并返回数据。 */
    subtitle_box finish_subtitle_read();

    /* 返回最后一个位置.微秒*/
    qint64 tell();
    bool eof();

    /* 移动到指定的位置，微秒。影响所有的流。确保位置不超出范围！ */
    void seek(qint64 pos);

    /* 完成后，关闭输入和清理。 */
    void close();
};

#endif // MEDIA_INPUT_H
