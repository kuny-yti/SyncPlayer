#ifndef VIDEO_DECODE_THREAD_H
#define VIDEO_DECODE_THREAD_H
#include "thread_base.h"
#include "type.h"
#include "video_frame.h"

//! 视频解码线程
//! 从数据队列中取出数据包并解码到数据帧中
class video_decode_thread : public thread
{
private:
    QString _url;
    class ffmpeg_stuff *_ffmpeg;
    struct ffmpeg_video *_video;
    int _stream;
    video_frame _frame;
    int _raw_frames;

    qint64 handle_timestamp(qint64 timestamp);
public:
    explicit video_decode_thread(const QString &url,
                                 class ffmpeg_stuff *ffmpeg,
                                 int stream);

    void set_raw_frames(int raw_frames)
    {
        _raw_frames = raw_frames;
    }
    void run();
    const video_frame &frame()
    {
        return _frame;
    }

};

#endif // VIDEO_DECODE_THREAD_H
