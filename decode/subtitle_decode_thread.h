#ifndef SUBTITLE_DECODE_THREAD_H
#define SUBTITLE_DECODE_THREAD_H

#include "thread_base.h"
#include "subtitle_box.h"

//! 字幕解码线程
class subtitle_decode_thread : public thread
{
private:
    QString _url;
    class ffmpeg_stuff *_ffmpeg;
    struct ffmpeg_subtitle *_subtitle;
    int _stream;
    subtitle_box _box;

    qint64 handle_timestamp(qint64 timestamp);
public:
    explicit subtitle_decode_thread(const QString &url,
                                    class ffmpeg_stuff *ffmpeg,
                                    int stream);

    void run();
    const subtitle_box &box()
    {
        return _box;
    }

};


#endif // SUBTITLE_DECODE_THREAD_H
