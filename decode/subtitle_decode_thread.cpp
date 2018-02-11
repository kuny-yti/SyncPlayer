#include "subtitle_decode_thread.h"
#include "ffmpeg.h"
#include <QDebug>

subtitle_decode_thread::subtitle_decode_thread(const QString &url,
                                               class ffmpeg_stuff *ffmpeg,
                                               int stream) :
    _url(url),
    _ffmpeg(ffmpeg),
    _stream(stream),
    _box(),
    _subtitle(&ffmpeg->subtitle)
{
}


qint64 subtitle_decode_thread::handle_timestamp(qint64 timestamp)
{
    qint64 ts = timestamp_helper(_subtitle->last_time[_stream], timestamp);
    _ffmpeg->pos = ts;
    return ts;
}

void subtitle_decode_thread::run()
{
    static AVPacket packet, tmppacket;
    if (_subtitle->box_buffers[_stream].empty())
    {
        bool empty;
        do
        {
            _subtitle->mutexes[_stream].lock();
            empty = _subtitle->packet[_stream].empty();
            _subtitle->mutexes[_stream].unlock();
            if (empty)
            {
                if (_ffmpeg->reader->eof())
                {
                    _box = subtitle_box();
                    return;
                }
               DEBUG(_url + ": subtitle stream " + QString::number(_stream) + ": need to wait for packets...");
                _ffmpeg->reader->start();
                _ffmpeg->reader->finish();
            }
        }
        while (empty);
        _subtitle->mutexes[_stream].lock();
        packet = _subtitle->packet[_stream].front();
        _subtitle->packet[_stream].pop_front();
        _subtitle->mutexes[_stream].unlock();
        _ffmpeg->reader->start();

        qint64 timestamp = packet.pts * 1000000
            * _ffmpeg->f_ctx->streams[_subtitle->streams[_stream]]->time_base.num
            / _ffmpeg->f_ctx->streams[_subtitle->streams[_stream]]->time_base.den;

        AVSubtitle subtitle;
        int got_subtitle;
        tmppacket = packet;

        if (_subtitle->ctxs[_stream]->codec_id == CODEC_ID_TEXT)
        {
            qint64 duration = packet.convergence_duration * 1000000
                * _ffmpeg->f_ctx->streams[_subtitle->streams[_stream]]->time_base.num
                / _ffmpeg->f_ctx->streams[_subtitle->streams[_stream]]->time_base.den;

            // Put it in the subtitle buffer
            subtitle_box box = _subtitle->templates[_stream];
            box.p_start_time = timestamp;
            box.p_stop_time = timestamp + duration;

            box.format = FORMAT_TEXT;
            box.str = reinterpret_cast<const char *>(packet.data);

            _subtitle->box_buffers[_stream] << (box);

            tmppacket.size = 0;
        }

        while (tmppacket.size > 0)
        {
            int len = avcodec_decode_subtitle2(_subtitle->ctxs[_stream],
                    &subtitle, &got_subtitle, &tmppacket);
            if (len < 0)
            {
                tmppacket.size = 0;
                break;
            }
            tmppacket.data += len;
            tmppacket.size -= len;
            if (!got_subtitle)
            {
                continue;
            }
            // Put it in the subtitle buffer
            subtitle_box box = _subtitle->templates[_stream];
            box.p_start_time = timestamp + subtitle.start_display_time * 1000;
            box.p_stop_time = box.p_start_time + subtitle.end_display_time * 1000;
            for (unsigned int i = 0; i < subtitle.num_rects; i++)
            {
                AVSubtitleRect *rect = subtitle.rects[i];
                switch (rect->type)
                {
                case SUBTITLE_BITMAP:
                    box.format = FORMAT_IMAGE;
                    box.images << (subtitle_box::image_t());
                    box.images.back().size.setWidth(rect->w) ;
                    box.images.back().size.setHeight(rect->h);
                    box.images.back().pos.setX(rect->x) ;
                    box.images.back().pos.setY(rect->y);
                    box.images.back().palette.resize(4 * rect->nb_colors);
                    memcpy(&(box.images.back().palette[0]), rect->pict.data[1],
                            box.images.back().palette.size());
                    box.images.back().linesize = rect->pict.linesize[0];
                    box.images.back().data.resize(box.images.back().size.height()
                                                  * box.images.back().linesize);
                    memcpy(&(box.images.back().data[0]), rect->pict.data[0],
                            box.images.back().data.size() * sizeof(uint8_t));
                    break;
                case SUBTITLE_TEXT:
                    box.format = FORMAT_TEXT;
                    if (!box.str.isEmpty())
                    {
                        box.str += '\n';
                    }
                    box.str += rect->text;
                    break;
                case SUBTITLE_ASS:
                    box.format = FORMAT_ASS;
                    box.style = QString::fromStdString(std::string(reinterpret_cast<const char *>(
                                _subtitle->ctxs[_stream]->subtitle_header),
                            _subtitle->ctxs[_stream]->subtitle_header_size));
                    if (!box.str.isEmpty())
                    {
                        box.str += '\n';
                    }
                    box.str += rect->ass;
                    break;
                case SUBTITLE_NONE:
                    // Should never happen, but make sure we have a valid subtitle box anyway.
                    box.format = FORMAT_TEXT;
                    box.str = ' ';
                    break;
                }
            }
            _subtitle->box_buffers[_stream] << (box);
            avsubtitle_free(&subtitle);
        }

        av_free_packet(&packet);
    }
    if (_subtitle->box_buffers[_stream].empty())
    {
        _box = subtitle_box();
        return;
    }
    _box = _subtitle->box_buffers[_stream].front();
    _subtitle->box_buffers[_stream].pop_front();
}
