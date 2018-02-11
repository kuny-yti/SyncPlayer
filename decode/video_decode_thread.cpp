#include "video_decode_thread.h"
#include "ffmpeg.h"
#include <QDebug>

video_decode_thread::video_decode_thread(const QString &url,
                                         class ffmpeg_stuff *ffmpeg,
                                         int stream) :
    _url(url),
    _ffmpeg(ffmpeg),
    _stream(stream),
    _frame(),
    _raw_frames(1),
    _video(&ffmpeg->video)
{
}


qint64 video_decode_thread::handle_timestamp(qint64 timestamp)
{
    qint64 ts = timestamp_helper(_video->last_time[_stream], timestamp);
    if (!_ffmpeg->have_active_audio_stream || _ffmpeg->pos == 0UL)
    {
        _ffmpeg->pos = ts;
    }
    return ts;
}

void video_decode_thread::run()
{
    _frame = _video->templates[_stream];
    for (int raw_frame = 0; raw_frame < _raw_frames; raw_frame++)
    {
        int frame_finished = 0;
read_frame:
        do
        {
            bool empty;
            do
            {
                _video->mutexes[_stream].lock();
                empty = _video->packet[_stream].empty();
                _video->mutexes[_stream].unlock();
                if (empty)
                {
                    if (_ffmpeg->reader->eof())//当读取有错误时或是视频播放完毕时走此路
                    {
                        if (raw_frame == 1)
                        {
                            _frame.data[1][0] = _frame.data[0][0];
                            _frame.data[1][1] = _frame.data[0][1];
                            _frame.data[1][2] = _frame.data[0][2];
                            _frame.line_size[1][0] = _frame.line_size[0][0];
                            _frame.line_size[1][1] = _frame.line_size[0][1];
                            _frame.line_size[1][2] = _frame.line_size[0][2];
                        }
                        else
                        {
                            _frame = video_frame();
                        }
                        return;
                    }
                    DEBUG(_url + ": video stream " + QString::number(_stream)
                               + ": need to wait for packets...");
                    _ffmpeg->reader->start();
                    _ffmpeg->reader->finish();
                }
            }
            while (empty);//以上步骤是若队列为空时，向队列添加数据包

            av_free_packet(&(_video->packets[_stream]));

            //锁定队列，从队列中取出一帧数据
            _video->mutexes[_stream].lock();
            _video->packets[_stream] = _video->packet[_stream].front();
            _video->packet[_stream].pop_front();
            _video->mutexes[_stream].unlock();

            _ffmpeg->reader->start();       // 在队列中补充数据

            //解码视频数据
            avcodec_decode_video2(_video->ctxs[_stream],
                    _video->frames[_stream], &frame_finished,
                    &(_video->packets[_stream]));
        }
        while (!frame_finished);

        if ((_video->frames[_stream]->width !=
                _video->templates[_stream].raw_size.width())
                || (_video->frames[_stream]->height !=
                _video->templates[_stream].raw_size.height()))
        {
            DEBUG(QString("%1 video stream %2: Dropping %3x%4 frame").
                   arg(_url.toLatin1().constData()).
                   arg(_stream + 1).arg(_video->frames[_stream]->width).
                   arg(_video->frames[_stream]->height));
            goto read_frame;
        }
        //若数据布局为BGRA则进行转换
        if (_frame.layout == LAYOUT_BGRA32)
        {
            sws_scale(_video->sws_ctxs[_stream],
                    _video->frames[_stream]->data,
                    _video->frames[_stream]->linesize,
                    0, _frame.raw_size.height(),
                    _video->sws_frames[_stream]->data,
                    _video->sws_frames[_stream]->linesize);

            _frame.data[raw_frame][0] = _video->sws_frames[_stream]->data[0];
            _frame.line_size[raw_frame][0] = _video->sws_frames[_stream]->linesize[0];
        }
        else //若不转换则拷贝帧数据
        {
            const AVFrame *src_frame = _video->frames[_stream];
            if (_raw_frames == 2 && raw_frame == 0)
            {
                // We need to buffer the data because FFmpeg will clubber it when decoding the next frame.
                av_picture_copy(reinterpret_cast<AVPicture *>(_video->buffered_frames[_stream]),
                        reinterpret_cast<AVPicture *>(_video->frames[_stream]),
                        static_cast<enum PixelFormat>(_video->ctxs[_stream]->pix_fmt),
                        _video->ctxs[_stream]->width,
                        _video->ctxs[_stream]->height);
                src_frame = _video->buffered_frames[_stream];
            }
            _frame.data[raw_frame][0] = src_frame->data[0];
            _frame.data[raw_frame][1] = src_frame->data[1];
            _frame.data[raw_frame][2] = src_frame->data[2];
            _frame.line_size[raw_frame][0] = src_frame->linesize[0];
            _frame.line_size[raw_frame][1] = src_frame->linesize[1];
            _frame.line_size[raw_frame][2] = src_frame->linesize[2];
        }

        //处理演示时间戳
        if (_video->packets[_stream].dts != static_cast<qint64>(AV_NOPTS_VALUE))
        {
            _frame.p_time = handle_timestamp(_video->packets[_stream].dts * 1000000
                    * _ffmpeg->f_ctx->streams[_video->streams[_stream]]->time_base.num
                    / _ffmpeg->f_ctx->streams[_video->streams[_stream]]->time_base.den);
        }
        else if (_video->last_time[_stream] != 0UL)
        {
            DEBUG(_url + ": video stream " + QString::number(_stream)
                    + ": no timestamp available, using a questionable guess");
            _frame.p_time = _video->last_time[_stream];
        }
        else
        {
           DEBUG(_url + ": video stream " + QString::number(_stream)
                    + ": no timestamp available, using a bad guess");
            _frame.p_time = _ffmpeg->pos;
        }
    }
}
