#include "audio_decode_thread.h"
#include "ffmpeg.h"
#include<stdlib.h>
#include <QDebug>

//!处理时间戳
qint64 audio_decode_thread::handle_timestamp(qint64 timestamp)
{
    qint64 ts = timestamp_helper(_audio->last_time[_stream],
                                 timestamp);
    _ffmpeg->pos = ts;
    return ts;
}

audio_decode_thread::audio_decode_thread(const QString &url,
                                         ffmpeg_stuff *ffmpeg,
                                         int stream):
    _url(url),
    _ffmpeg(ffmpeg),
    _stream(stream),
    _audio(&ffmpeg->audio)
{
}

void audio_decode_thread::run()
{
    static AVPacket packet, tmppacket;

    int size = _audio->blobs[_stream].size();
    void *buffer = _audio->blobs[_stream].ptr();

    qint64 timestamp = 0UL;
    int i = 0;

    while (i < size)
    {
        if (_audio->buffers[_stream].size() > 0)
        {
            int remaining = min((size - i), _audio->buffers[_stream].size());
            memcpy(buffer, &(_audio->buffers[_stream][0]), remaining);
            _audio->buffers[_stream].erase(
                        _audio->buffers[_stream].begin(),
                        _audio->buffers[_stream].begin() + remaining);
            buffer = reinterpret_cast<uchar *>(buffer) + remaining;
            i += remaining;
        }

        //! 从队列中读取音频数据
        if (_audio->buffers[_stream].size() == 0)
        {
            bool empty;
            do
            {
                _audio->mutexes[_stream].lock();
                empty = _audio->packet[_stream].empty();
                _audio->mutexes[_stream].unlock();
                if (empty)
                {
                    if (_ffmpeg->reader->eof())
                    {
                        _blob = audio_blob();
                        return;
                    }
                    DEBUG((_url + ": audio stream " +
                           QString::number(_stream) + ": need to wait for packets..."));
                    _ffmpeg->reader->start();
                    _ffmpeg->reader->finish();
                }
            }
            while (empty);

            _audio->mutexes[_stream].lock();
            packet = _audio->packet[_stream].front();
            _audio->packet[_stream].pop_front();
            _audio->mutexes[_stream].unlock();

            _ffmpeg->reader->start();   // 补充队列

            if (timestamp == 0UL && packet.dts !=
                    static_cast<qint64>(AV_NOPTS_VALUE))
            {
                timestamp = packet.dts * 1000000
                    * _ffmpeg->f_ctx->streams[_audio->streams[_stream]]->time_base.num
                    / _ffmpeg->f_ctx->streams[_audio->streams[_stream]]->time_base.den;
            }

            // 解码音频数据包
            AVFrame audioframe ;//= { { 0 } };
            memset (&audioframe, 0, sizeof(AVFrame));
            tmppacket = packet;

            while (tmppacket.size > 0)
            {
                int got_frame = 0;
                int len = avcodec_decode_audio4(_audio->ctxs[_stream],
                        &audioframe, &got_frame, &tmppacket);
                if (len < 0)
                {
                    tmppacket.size = 0;
                    break;
                }
                tmppacket.data += len;
                tmppacket.size -= len;
                if (!got_frame)
                {
                    continue;
                }
                int plane_size;
                int tmpbuf_size = av_samples_get_buffer_size(&plane_size,
                        _audio->ctxs[_stream]->channels,
                        audioframe.nb_samples,
                        _audio->ctxs[_stream]->sample_fmt, 1);

                if (av_sample_fmt_is_planar(_audio->ctxs[_stream]->sample_fmt)
                        && _audio->ctxs[_stream]->channels > 1)
                {
                    int dummy;
                    int sample_size = av_samples_get_buffer_size(&dummy, 1, 1,
                            _audio->ctxs[_stream]->sample_fmt, 1);
                    quint8 *out = reinterpret_cast<quint8 *>(&(_audio->tmpbufs[_stream][0]));
                    for (int s = 0; s < audioframe.nb_samples; s++)
                    {
                        for (int c = 0; c < _audio->ctxs[_stream]->channels; c++)
                        {
                            memcpy(out, audioframe.extended_data[c] + s * sample_size, sample_size);
                            out += sample_size;
                        }
                    }
                }
                else
                {
                    memcpy(&(_audio->tmpbufs[_stream][0]), audioframe.extended_data[0], plane_size);
                }
                // 把它放在解码音频数据缓冲区
                if (_audio->ctxs[_stream]->sample_fmt == AV_SAMPLE_FMT_S32)
                {
                    // we need to convert this to AV_SAMPLE_FMT_FLT
                    Q_ASSERT(sizeof(qint32) == sizeof(float));
                    Q_ASSERT(tmpbuf_size % sizeof(qint32) == 0);
                    void *tmpbuf_v = static_cast<void *>(&(_audio->tmpbufs[_stream][0]));
                    qint32 *tmpbuf_i32 = static_cast<qint32 *>(tmpbuf_v);
                    float *tmpbuf_flt = static_cast<float *>(tmpbuf_v);
                    const float posdiv = +static_cast<float>(0xffffffff);
                    const float negdiv = -static_cast<float>(0UL);
                    for (int j = 0; j < (int)(tmpbuf_size / sizeof(qint32)); j++)
                    {
                        qint32 sample_i32 = tmpbuf_i32[j];
                        float sample_flt = sample_i32 / (sample_i32 >= 0 ? posdiv : negdiv);
                        tmpbuf_flt[j] = sample_flt;
                    }
                }
                int old_size = _audio->buffers[_stream].size();
                _audio->buffers[_stream].resize(old_size + tmpbuf_size);
                memcpy(&(_audio->buffers[_stream][old_size]), _audio->tmpbufs[_stream], tmpbuf_size);
            }

            av_free_packet(&packet);
        }
    }
    if (timestamp == 0UL)
    {
        timestamp = _audio->last_time[_stream];
    }
    if (timestamp == 0UL)
    {
        DEBUG((_url + ": audio stream " + QString::number(_stream)
               + ": no timestamp available, using a bad guess"));
        timestamp = _ffmpeg->pos;
    }

    _blob = _audio->templates[_stream];
    _blob.data = _audio->blobs[_stream].ptr();
    _blob.size = _audio->blobs[_stream].size();
    _blob.p_time = handle_timestamp(timestamp);

}
