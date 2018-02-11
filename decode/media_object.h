#ifndef MEDIA_OBJECT_H
#define MEDIA_OBJECT_H
#include "type.h"
#include <QString>
#include <QVector>
#include "ffmpeg.h"
#include "device_request.h"
#include "video_frame.h"
#include "audio_blob.h"
#include "subtitle_box.h"
#include "blob.h"

//!
//! \brief The media_object class
//! 负责媒体文件的解码
//!
class media_object_p;
class media_object
{
private:
    media_object_p *_dptr;

    // 线程的运行，可以访问私有成员
    friend class read_thread;
    friend class video_decode_thread;
    friend class audio_decode_thread;
    friend class subtitle_decode_thread;

    void print_open_news();
public:
    media_object(bool convert_bgra32 = false);
    ~media_object();

    /* 打开媒体对象，url可能是文件 */
    void open(const QString &url, const device_request &dev_request = device_request());

    /* 获取元数据 */
    const QString &url() const;
    long tags() const;
    const QString &tag_name(long i) const;
    const QString &tag_value(long i) const;
    const QString &tag_value(const QString &tag_name) const;

    /* 获取文件中媒体流数目 */
    int video_streams() const;
    int audio_streams() const;
    int subtitle_streams() const;

    /* 激活使用的媒体流。无效的流将无法访问。 */
    void video_stream_set_active(int video_stream, bool active);
    void audio_stream_set_active(int audio_stream, bool active);
    void subtitle_stream_set_active(int subtitle_stream, bool active);

    /* 获得视频流信息。 */
    const video_frame &video_template(int video_stream) const;
    // 返回的帧速率。
    int video_rate_numerator(int video_stream) const;
    int video_rate_denominator(int video_stream) const;
    // 视频流持续的时间(微妙)
    qint64 video_duration(int video_stream) const;

    /* 获取有关音频流信息。 */
    const audio_blob &audio_template(int audio_stream) const;
    // 音频流持续的时间(微妙)
    qint64 audio_duration(int audio_stream) const;

    /* 获取有关字幕流信息。 */
    const subtitle_box &subtitle_template(int subtitle_stream) const;
    // 字幕流持续的时间(微妙)
    qint64 subtitle_duration(int subtitle_stream) const;

    /*
     * 访问媒体数据
     */
    /* 开始读取一个视频帧(异步的，在单独线程中)
    “raw_frames”的值必须是1或2。如果是2，由两个连续的原始视频帧*/
    void start_video_read(int video_stream, int raw_frames);
    /* 等待视频帧读取完成，并返回视频帧*/
    video_frame finish_video_read(int video_stream);

    /* 开始读取一个音频数据(异步的，在单独线程中)*/
    void start_audio_read(int audio_stream, int size);
    /* 等待音频读取完成，并返回音频数据 */
    audio_blob finish_audio_read(int audio_stream);

    /* 开始读取一个字幕(异步的，在单独线程中) */
    void start_subtitle_read(int subtitle_stream);
    /* 等待字幕读取完成，并返回字幕数据 */
    subtitle_box finish_subtitle_read(int subtitle_stream);

    /* 返回数据流最后的时间(微妙)*/
    qint64 tell();
    bool eof();

    /* 移动位置(微妙) */
    void seek(qint64 pos);
    /*
     * 清楚数据和对象
     */
    void close();
};

#endif // MEDIA_OBJECT_H
