#ifndef AUDIO_DECODE_THREAD_H
#define AUDIO_DECODE_THREAD_H

#include "thread_base.h"
#include "audio_blob.h"

//! 音频解码线程
//！ 从数据队列中取出数据包并解码到音频数据中
class audio_decode_thread : public thread
{
private:
    QString         _url;
    class ffmpeg_stuff *  _ffmpeg;
    struct ffmpeg_audio * _audio;
    int             _stream;
    audio_blob      _blob;

    qint64 handle_timestamp(qint64 timestamp);
public:
    explicit audio_decode_thread(const QString &url,
                                 class ffmpeg_stuff *ffmpeg,
                                 int stream);

    void run();
    const audio_blob &blob()
    {
        return _blob;
    }

};


#endif // AUDIO_DECODE_THREAD_H
