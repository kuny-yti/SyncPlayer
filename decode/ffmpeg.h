#ifndef FFMEG_H
#define FFMEG_H

#if HAVE_SYSCONF
#  include <unistd.h>
#else
#  include <windows.h>
#endif
#include <QQueue>
#include <QFileInfo>
#include <cmath>
#include <QDebug>

#include "type.h"
#include "blob.h"
#include "read_thread.h"
#include "video_frame.h"
#include "video_decode_thread.h"
#include "audio_blob.h"
#include "audio_decode_thread.h"
#include "subtitle_box.h"
#include "subtitle_decode_thread.h"
#include "thread_base.h"

#ifdef __cplusplus
extern "C"
{
#endif /*__cplusplus*/
    #include <libavformat/avformat.h>
    #include <libavdevice/avdevice.h>
    #include <libavcodec/avcodec.h>
    #include <libswscale/swscale.h>
    #include <libavfilter/avfilter.h>
    #include <libavutil/avutil.h>
    #include <libswresample/swresample.h>

    #include <libavutil/dict.h>
    #include <libavutil/log.h>
    #include <libavutil/mathematics.h>
    #include <libavutil/cpu.h>
    #include <libavutil/error.h>
    #include <libavutil/opt.h>
    #include <libavutil/pixdesc.h>

    #include <libavutil/samplefmt.h>
    #include <libavfilter/avfilter.h>
    #include <libavfilter/buffersink.h>
    #include <libavfilter/buffersrc.h>
#ifdef __cplusplus
}
#endif /*__cplusplus*/

static const int max_audio_frame_size = 192000; // 1秒32位48kHz的音频
static const int audio_tmpbuf_size = (max_audio_frame_size * 3) / 2;

typedef struct ffmpeg_audio
{
    QVector<int> streams;
    QVector<AVCodecContext *> ctxs;
    QVector<audio_blob> templates;
    QVector<AVCodec *> codecs;
    QVector<QQueue<AVPacket> > packet;
    QVector<mutex> mutexes;
    QVector<audio_decode_thread*> threads;
    QVector<uchar *> tmpbufs;
    QVector<blob> blobs;
    QVector<QVector<uchar> > buffers;
    QVector<qint64> last_time;
}ffmpeg_audio;

typedef struct ffmpeg_video
{
    QVector<int> streams;
    QVector<AVCodecContext *> ctxs;
    QVector<video_frame> templates;
    QVector<struct SwsContext *> sws_ctxs;
    QVector<AVCodec *> codecs;
    QVector<QQueue<AVPacket> > packet;
    QVector<mutex> mutexes;
    QVector<AVPacket> packets;
    QVector<video_decode_thread*> threads;
    QVector<AVFrame *> frames;
    QVector<AVFrame *> buffered_frames;
    QVector<quint8 *> buffers;
    QVector<AVFrame *> sws_frames;
    QVector<quint8 *> sws_buffers;
    QVector<qint64> last_time;
}ffmpeg_video;

typedef struct ffmpeg_subtitle
{
    QVector<int> streams;
    QVector<AVCodecContext *> ctxs;
    QVector<subtitle_box> templates;
    QVector<AVCodec *> codecs;
    QVector<QQueue<AVPacket> > packet;
    QVector<mutex> mutexes;
    QVector<subtitle_decode_thread*> threads;
    QVector<QQueue<subtitle_box> > box_buffers;
    QVector<qint64> last_time;
}ffmpeg_subtitle;

class ffmpeg_stuff
{
public:
    AVFormatContext *f_ctx;
    read_thread *reader;

    bool have_active_audio_stream;
    qint64 pos;

    ffmpeg_audio audio;
    ffmpeg_video video;
    ffmpeg_subtitle subtitle;
};

// 使用一个解码线程每处理器的视频解码。
inline static int video_decoding_threads()
{
    static long n = -1;
    if (n < 0)
    {
#ifdef HAVE_SYSCONF
        n = sysconf(_SC_NPROCESSORS_ONLN);
#else
        SYSTEM_INFO si;
        GetSystemInfo(&si);
        n = si.dwNumberOfProcessors;
#endif
        if (n < 1)
        {
            n = 1;
        }
        else if (n > 16)
        {
            n = 16;
        }
    }
    return n;
}

// 返回 FFmpeg 错误到QString.
inline static QString my_av_strerror(int err)
{
    blob b(1024);
    av_strerror(err, b.ptr<char>(), b.size());
    return QString(b.ptr<const char>());
}

// 将 FFmpeg日志消息转换出来
inline static void my_av_log(void *ptr, int level, const char *fmt, va_list vl)
{
    static mutex line_mutex;
    static QString line;
    if (level > av_log_get_level())
    {
        return;
    }

    line_mutex.lock();
    QString p;
    AVClass* avc = ptr ? *reinterpret_cast<AVClass**>(ptr) : NULL;
    if (avc)
    {
        p = QString("[%1 @ %2] ").arg(avc->item_name(ptr)).arg( *(uint*)ptr);
    }
    QString s = s.vsprintf(fmt, vl);
    bool line_ends = false;
    if (s.length() > 0)
    {
        if (s[s.length() - 1] == '\n')
        {
            line_ends = true;
            s[s.length() - 1] = 0;
        }
    }
    line += s;
    if (line_ends)
    {
        QString meg;
        switch (level)
        {
        case AV_LOG_PANIC:
        case AV_LOG_FATAL:
        case AV_LOG_ERROR:
            meg = "error:";
            break;
        case AV_LOG_WARNING:
            meg = "warning:";
            break;
        case AV_LOG_INFO:
        case AV_LOG_VERBOSE:
        case AV_LOG_DEBUG:
        default:
            meg = "debug:";
            break;
        }
        qDebug () << (meg+QString("FFmpeg: ") + p + line);
        line.clear();
    }
    line_mutex.unlock();
}

// 处理时间戳
inline static qint64 timestamp_helper(qint64 &last_timestamp, qint64 timestamp)
{
    if (timestamp == 0UL)
    {
        timestamp = last_timestamp;
    }
    last_timestamp = timestamp;
    return timestamp;
}

// 得到一个流持续时间
inline static qint64 stream_duration(AVStream *stream, AVFormatContext *format)
{
    qint64 duration = stream->duration;
    if (duration > 0)
    {
        AVRational time_base = stream->time_base;
        return duration * 1000000 * time_base.num / time_base.den;
    }
    else
    {
        duration = format->duration;
        return duration * 1000000 / AV_TIME_BASE;
    }
}

// 把一个文件名（或URL）的扩展
inline static QString get_extension(const QString& url)
{
    QFileInfo file(url);
    return file.suffix().toLower();
}

#endif // FFMEG_H
