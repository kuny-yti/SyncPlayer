#include "read_thread.h"
#include "ffmpeg.h"
#include <QDebug>

class read_thread_p
{
public :
    const QString _url;
    const bool _is_device;
    ffmpeg_stuff *_ffmpeg;
    ffmpeg_audio *_audio;
    ffmpeg_video *_video;
    ffmpeg_subtitle *_subtitle;

    bool _eof;
    read_thread_p(const QString &url, bool is_device,
                  class ffmpeg_stuff *ffmpeg):
        _url(url),
        _is_device(is_device),
        _ffmpeg(ffmpeg),
        _audio(&ffmpeg->audio),
        _video(&ffmpeg->video),
        _subtitle(&ffmpeg->subtitle)
    {
    }
    bool is_read()
    {
        //根据设备类型选择队列数量
        const int video_low_threshold = (_is_device ? 1 : 2);
        const int audio_low_threshold = (_is_device ? 1 : 5);
        const int subtitle_low_threshold = (_is_device ? 1 : 1);
        bool need_another = false;

        //! 处理是否有读取可读取的数据包 (need_another = true需要读取)
        for (int i = 0; (!need_another) && (i < _video->streams.size()); i++)
        {
            if (_ffmpeg->f_ctx->streams[_video->streams[i]]->discard == AVDISCARD_DEFAULT)
            {
                _video->mutexes[i].lock();
                need_another = _video->packet[i].size() < video_low_threshold;
                _video->mutexes[i].unlock();
            }
        }
        for (int i = 0; (!need_another) && (i < _audio->streams.size()); i++)
        {
            if (_ffmpeg->f_ctx->streams[_audio->streams[i]]->discard == AVDISCARD_DEFAULT)
            {
                _audio->mutexes[i].lock();
                need_another = _audio->packet[i].size() < audio_low_threshold;
                _audio->mutexes[i].unlock();
            }
        }
        for (int i = 0; (!need_another) && i < (_subtitle->streams.size()); i++)
        {
            if (_ffmpeg->f_ctx->streams[_subtitle->streams[i]]->discard == AVDISCARD_DEFAULT)
            {
                _subtitle->mutexes[i].lock();
                need_another = _subtitle->packet[i].size() < subtitle_low_threshold;
                _subtitle->mutexes[i].unlock();
            }
        }
        if (!need_another)
        {
            DEBUG(_url + ": No need to read more packets.");
            return false;
        }

        return true;
    }
    bool push_video(AVPacket &packet)
    {
        //! 把数据包添加到队列中
        bool packet_queued = false;
        //! 添加视频数据包到视频队列中
        for (int i = 0; (i < _video->streams.size()) && (!packet_queued); i++)
        {
            if (packet.stream_index == _video->streams[i])
            {
                if (av_dup_packet(&packet) < 0)
                {
                    DEBUG( _url+QString(": Cannot duplicate packet."));
                }
                _video->mutexes[i].lock();
                _video->packet[i] << (packet);
                _video->mutexes[i].unlock();
                packet_queued = true;
                DEBUG(_url + ": "
                        + QString::number(_video->packet[i].size())
                        + " packets queued in video stream " + QString::number(i) + ".");
            }
        }
        return packet_queued;
    }
    bool push_audio(AVPacket &packet)
    {
        bool packet_queued = false;
        //! 添加音频数据包到音频队列中
        for (int i = 0; i < _audio->streams.size() && !packet_queued; i++)
        {
            if (packet.stream_index == _audio->streams[i])
            {
                _audio->mutexes[i].lock();
                if (_audio->packet[i].empty()
                        && _audio->last_time[i] == 0UL
                        && packet.dts == static_cast<int64_t>(AV_NOPTS_VALUE))
                {
                    DEBUG(_url + ": audio stream " + QString::number(i)
                            + ": dropping packet because it has no timestamp");
                }
                else
                {
                    if (av_dup_packet(&packet) < 0)
                    {
                        _audio->mutexes[i].unlock();
                        DEBUG(_url+QString(": Cannot duplicate packet."));
                    }
                    _audio->packet[i] << (packet);
                    packet_queued = true;
                    DEBUG(_url + ": "
                            + QString::number(_audio->packet[i].size())
                            + " packets queued in audio stream " + QString::number(i) + ".");
                }
                _audio->mutexes[i].unlock();
            }
        }
        return packet_queued;
    }
    bool push_subtitle(AVPacket &packet)
    {
        bool packet_queued = false;
        //! 添加字幕数据包到字幕队列中
        for (int i = 0; i < _subtitle->streams.size() && !packet_queued; i++)
        {
            if (packet.stream_index == _subtitle->streams[i])
            {
                _subtitle->mutexes[i].lock();
                if (_subtitle->packet[i].empty()
                        && _subtitle->last_time[i] == 0UL
                        && packet.dts == static_cast<int64_t>(AV_NOPTS_VALUE))
                {
                    DEBUG(_url + ": subtitle stream " + QString::number(i)
                            + ": dropping packet because it has no timestamp");
                }
                else
                {
                    if (av_dup_packet(&packet) < 0)
                    {
                        _subtitle->mutexes[i].unlock();
                        DEBUG(_url+QString(": Cannot duplicate packet."));
                    }
                    _subtitle->packet[i] << (packet);
                    packet_queued = true;
                    DEBUG(_url + ": "
                            + QString::number(_subtitle->packet[i].size())
                            + " packets queued in subtitle stream " + QString::number(i) + ".");
                }
                _subtitle->mutexes[i].unlock();
            }
        }
        return packet_queued;
    }
};

read_thread::read_thread(const QString &url, bool is_device,
                         class ffmpeg_stuff *ffmpeg)
{
     _dptr = new read_thread_p(url, is_device, ffmpeg);

     _dptr->_eof = false;
}
bool read_thread::eof() const
{
    return _dptr->_eof;
}
void read_thread::run()
{
    static AVPacket packet;
    while (!_dptr->_eof)
    {
        bool packet_queued = false;

        //是否要读取数据
        if (!_dptr->is_read())
        {
            break;
        }
        //! 读取数据包
        DEBUG(_dptr->_url + ": Reading a packet.");

        int e = av_read_frame(_dptr->_ffmpeg->f_ctx, &packet);
        if (e < 0)
        {
            if (e == AVERROR_EOF)
            {
                DEBUG(_dptr->_url + ": EOF.");
                _dptr->_eof = true;
                return;
            }
            else
            {
                DEBUG(_dptr->_url+ my_av_strerror(e));
            }
        }

        if (_dptr->push_video(packet))
        {
            packet_queued = true;
        }
        else if (_dptr->push_audio(packet))
        {
             packet_queued = true;
        }
        else if (_dptr->push_subtitle(packet))
        {
             packet_queued = true;
        }

        if (!packet_queued)
        {
            av_free_packet(&packet);
        }
    }
}
void read_thread::reset()
{
    _dptr->_eof = false;
}
