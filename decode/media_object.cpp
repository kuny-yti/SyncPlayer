#include "media_object.h"
#include <QDebug>

class media_object_p
{
public:
    bool _convert_bgra32;     // 总是转换视频格式到bgra32格式
    QString _url;                       // 媒体对象的URL(或是文件)
    bool _is_device;                    // 网址是否代表一个设备（例如数码相机）
    class ffmpeg_stuff *_ffmpeg;       // FFmpeg 相关数据
    QVector<QString> _tag_names;        // 元数据: 标签名称
    QVector<QString> _tag_values;       // 元数据: 标签值

    // 从指定流中提取视频帧和音频数据模板信息
    void set_video_template(int video_stream,
                                  int width_avcodec,
                                  int height_avcodec);
    void set_audio_template(int audio_stream);
    void set_subtitle_template(int subtitle_stream);

    // 根据输入打开媒体
    AVInputFormat *device_open(const device_request &dev_request,
                               AVDictionary **iparams);
    void decoder_open();
    void queues_alloc();
    void metadata();
    void dump_format();

    // 根据输入处理模板帧
    void frame_template_news(int index,
                             int width_avcodec,
                             int height_avcodec);
    void frame_template_color(int index);
    void frame_template_stereo(int index);

    // 停止所有线程和清除队列数据
    void stop_decode_thread_all();
    void clear_streams_queues();

    const QString &tag_value(const QString &tag_name) const;
};
media_object::media_object(bool convert_bgra32)
{
    _dptr = new media_object_p();
    _dptr->_convert_bgra32 = convert_bgra32;
    _dptr->_ffmpeg = 0;

    avdevice_register_all();
    av_register_all();
    avformat_network_init();

    /*switch (msg::level())
    {
    case msg::DBG:
        av_log_set_level(AV_LOG_DEBUG);
        break;
    case msg::INF:
        av_log_set_level(AV_LOG_INFO);
        break;
    case msg::WRN:
        av_log_set_level(AV_LOG_WARNING);
        break;
    case msg::ERR:
        av_log_set_level(AV_LOG_ERROR);
        break;
    case msg::REQ:
    default:
        av_log_set_level(AV_LOG_FATAL);
        break;
    }
    av_log_set_callback(my_av_log);*/
}
media_object::~media_object()
{
    if (_dptr->_ffmpeg)
    {
        close();
    }
    delete _dptr;
}

///////////////////////////////////////////////////////////////////////////////
/// 设置帧模板
void media_object_p::frame_template_news(int index,
                                       int width_avcodec,
                                       int height_avcodec)
{
    AVStream *video_stream = _ffmpeg->f_ctx->streams[_ffmpeg->video.streams[index]];
    AVCodecContext *video_codec_ctx = _ffmpeg->video.ctxs[index];
    video_frame &video_frame_template = _ffmpeg->video.templates[index];

    // 尺寸和纵横比
    video_frame_template.raw_size.setWidth(video_codec_ctx->width);
    video_frame_template.raw_size.setHeight(video_codec_ctx->height);

    if (width_avcodec >= 1
            && height_avcodec >= 1
            && width_avcodec <= video_codec_ctx->width
            && height_avcodec <= video_codec_ctx->height
            && (width_avcodec != video_codec_ctx->width
                || height_avcodec != video_codec_ctx->height))
    {
        DEBUG(QString("%1 video stream %2: using frame size %3x%4 instead of %5x%6.").
               arg(_url.toLatin1().constData()).
               arg(index + 1).
               arg(width_avcodec).
               arg(height_avcodec).
               arg(video_codec_ctx->width).
               arg(video_codec_ctx->height));
        video_frame_template.raw_size.setWidth(width_avcodec) ;
        video_frame_template.raw_size.setHeight(height_avcodec);
    }

    int ar_num = 1;
    int ar_den = 1;
    int ar_snum = video_stream->sample_aspect_ratio.num;
    int ar_sden = video_stream->sample_aspect_ratio.den;
    int ar_cnum = video_codec_ctx->sample_aspect_ratio.num;
    int ar_cden = video_codec_ctx->sample_aspect_ratio.den;
    if (ar_cnum > 0 && ar_cden > 0)
    {
        ar_num = ar_cnum;
        ar_den = ar_cden;
    }
    else if (ar_snum > 0 && ar_sden > 0)
    {
        ar_num = ar_snum;
        ar_den = ar_sden;
    }
    video_frame_template.raw_aspect_ratio =
        static_cast<float>(ar_num * video_frame_template.raw_size.width())
        / static_cast<float>(ar_den * video_frame_template.raw_size.height());

}
void media_object_p::frame_template_color(int index)
{
    AVCodecContext *video_codec_ctx = _ffmpeg->video.ctxs[index];
    video_frame &video_frame_template = _ffmpeg->video.templates[index];

    // 数据布局和颜色空间
    video_frame_template.layout = LAYOUT_BGRA32 ;
    video_frame_template.color_space = SPACE_SRGB;
    video_frame_template.value_range = RANGE_U8_FULL;
    video_frame_template.chroma_location = LOCATION_CENTER;
    if (!_convert_bgra32
            && (video_codec_ctx->pix_fmt == PIX_FMT_YUV444P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV444P10
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV422P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV422P10
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV420P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV420P10))
    {
        if (video_codec_ctx->pix_fmt == PIX_FMT_YUV444P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV444P10)
        {
            video_frame_template.layout = LAYOUT_YUV444P;
        }
        else if (video_codec_ctx->pix_fmt == PIX_FMT_YUV422P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV422P10)
        {
            video_frame_template.layout = LAYOUT_YUV422P;
        }
        else
        {
            video_frame_template.layout = LAYOUT_YUV420P;
        }
        video_frame_template.color_space = SPACE_YUV601;
        if (video_codec_ctx->colorspace == AVCOL_SPC_BT709)
        {
            video_frame_template.color_space = SPACE_YUV709;
        }
        if (video_codec_ctx->pix_fmt == PIX_FMT_YUV444P10
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV422P10
                || video_codec_ctx->pix_fmt == PIX_FMT_YUV420P10)
        {
            video_frame_template.value_range = RANGE_U10_MPEG;
            if (video_codec_ctx->color_range == AVCOL_RANGE_JPEG)
            {
                video_frame_template.value_range = RANGE_U10_FULL;
            }
        }
        else
        {
            video_frame_template.value_range = RANGE_U8_MPEG;
            if (video_codec_ctx->color_range == AVCOL_RANGE_JPEG)
            {
                video_frame_template.value_range = RANGE_U8_FULL;
            }
        }
        video_frame_template.chroma_location = LOCATION_CENTER;
        if (video_codec_ctx->chroma_sample_location == AVCHROMA_LOC_LEFT)
        {
            video_frame_template.chroma_location = LOCATION_LEFT;
        }
        else if (video_codec_ctx->chroma_sample_location == AVCHROMA_LOC_TOPLEFT)
        {
            video_frame_template.chroma_location = LOCATION_TOP_LEFT;
        }
    }
    else if (!_convert_bgra32
            && (video_codec_ctx->pix_fmt == PIX_FMT_YUVJ444P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUVJ422P
                || video_codec_ctx->pix_fmt == PIX_FMT_YUVJ420P))
    {
        if (video_codec_ctx->pix_fmt == PIX_FMT_YUVJ444P)
        {
            video_frame_template.layout = LAYOUT_YUV444P;
        }
        else if (video_codec_ctx->pix_fmt == PIX_FMT_YUVJ422P)
        {
            video_frame_template.layout = LAYOUT_YUV422P;
        }
        else
        {
            video_frame_template.layout = LAYOUT_YUV420P;
        }
        video_frame_template.color_space = SPACE_YUV601;
        video_frame_template.value_range = RANGE_U8_FULL;
        video_frame_template.chroma_location = LOCATION_CENTER;
    }

}
void media_object_p::frame_template_stereo(int index)
{
    QString val;
    video_frame &video_frame_template = _ffmpeg->video.templates[index];
    AVStream *video_stream = _ffmpeg->f_ctx->streams[_ffmpeg->video.streams[index]];
    // 立体布局
    video_frame_template.stereo_layout = STEREO_MONO;
    video_frame_template.stereo_layout_swap = false;

    /* 从分辨率确定立体布局*/
    if (video_frame_template.raw_size.width() / 2 > video_frame_template.raw_size.height())
    {
        video_frame_template.stereo_layout = STEREO_LEFT_RIGHT;
    }
    else if (video_frame_template.raw_size.height() > video_frame_template.raw_size.width())
    {
        video_frame_template.stereo_layout = STEREO_TOP_BOTTOM;
    }

    /* 从文件扩展名确定立体布局 */
    QString extension = get_extension(_url);
    if (extension == "mpo")
    {
        /*MPO文件:左右交替。 */
        video_frame_template.stereo_layout = STEREO_ALTERNATING;
    }
    else if (extension == "jps" || extension == "pns")
    {
        /* JPS 与 PNS :并排的左右模式*/
        video_frame_template.stereo_layout = STEREO_LEFT_RIGHT;
        video_frame_template.stereo_layout_swap = true;
    }

    /* 通过查看文件名确定输入模式。*/
    std::string url= _url.toStdString();
    std::string marker = url.substr(0, url.find_last_of('.'));
    uint last_dash = marker.find_last_of('-');
    if (last_dash != std::string::npos)
    {
        marker = marker.substr(last_dash + 1);
    }
    else
    {
        marker = "";
    }
    for (uint i = 0; i < marker.length(); i++)
    {
        marker[i] = std::tolower(marker[i]);
    }
    if (marker == "lr")
    {
        video_frame_template.stereo_layout = STEREO_LEFT_RIGHT;
        video_frame_template.stereo_layout_swap = false;
    }
    else if (marker == "rl")
    {
        video_frame_template.stereo_layout = STEREO_LEFT_RIGHT;
        video_frame_template.stereo_layout_swap = true;
    }
    else if (marker == "lrh" || marker == "lrq")
    {
        video_frame_template.stereo_layout = STEREO_LEFT_RIGHT_HALF;
        video_frame_template.stereo_layout_swap = false;
    }
    else if (marker == "rlh" || marker == "rlq")
    {
        video_frame_template.stereo_layout = STEREO_LEFT_RIGHT_HALF;
        video_frame_template.stereo_layout_swap = true;
    }
    else if (marker == "tb" || marker == "ab")
    {
        video_frame_template.stereo_layout = STEREO_TOP_BOTTOM;
        video_frame_template.stereo_layout_swap = false;
    }
    else if (marker == "bt" || marker == "ba")
    {
        video_frame_template.stereo_layout = STEREO_TOP_BOTTOM;
        video_frame_template.stereo_layout_swap = true;
    }
    else if (marker == "tbh" || marker == "abq")
    {
        video_frame_template.stereo_layout = STEREO_TOP_BOTTOM_HALF;
        video_frame_template.stereo_layout_swap = false;
    }
    else if (marker == "bth" || marker == "baq")
    {
        video_frame_template.stereo_layout = STEREO_TOP_BOTTOM_HALF;
        video_frame_template.stereo_layout_swap = true;
    }
    else if (marker == "eo")
    {
        video_frame_template.stereo_layout = STEREO_EVEN_ODD_ROWS;
        video_frame_template.stereo_layout_swap = false;
        // all image lines are given in this case, and there should be no interpolation [TODO]
    }
    else if (marker == "oe")
    {
        video_frame_template.stereo_layout = STEREO_EVEN_ODD_ROWS;
        video_frame_template.stereo_layout_swap = true;
        // all image lines are given in this case, and there should be no interpolation [TODO]
    }
    else if (marker == "eoq" || marker == "3dir")
    {
        video_frame_template.stereo_layout = STEREO_EVEN_ODD_ROWS;
        video_frame_template.stereo_layout_swap = false;
    }
    else if (marker == "oeq" || marker == "3di")
    {
        video_frame_template.stereo_layout = STEREO_EVEN_ODD_ROWS;
        video_frame_template.stereo_layout_swap = true;
    }
    else if (marker == "2d")
    {
        video_frame_template.stereo_layout = STEREO_MONO;
        video_frame_template.stereo_layout_swap = false;
    }

    val = tag_value("StereoscopicLayout");
    if (val == "SideBySideRF" || val == "SideBySideLF")
    {
        video_frame_template.stereo_layout_swap = (val == "SideBySideRF");
        val = tag_value("StereoscopicHalfWidth");
        video_frame_template.stereo_layout = (val == "1" ? STEREO_LEFT_RIGHT_HALF : STEREO_LEFT_RIGHT);

    }
    else if (val == "OverUnderRT" || val == "OverUnderLT")
    {
        video_frame_template.stereo_layout_swap = (val == "OverUnderRT");
        val = tag_value("StereoscopicHalfHeight");
        video_frame_template.stereo_layout = (val == "1" ? STEREO_TOP_BOTTOM_HALF : STEREO_TOP_BOTTOM);
    }
    val = "";
    AVDictionaryEntry *tag = NULL;
    while ((tag = av_dict_get(video_stream->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
        if (QString(tag->key) == "stereo_mode")
        {
            val = tag->value;
            break;
        }
    }
    if (val == "mono")
    {
        video_frame_template.stereo_layout = STEREO_MONO;
        video_frame_template.stereo_layout_swap = false;
    }
    else if (val == "left_right" || val == "right_left")
    {
        if (video_frame_template.raw_size.width() / 2 > video_frame_template.raw_size.height())
        {
            video_frame_template.stereo_layout = STEREO_LEFT_RIGHT;
        }
        else
        {
            video_frame_template.stereo_layout = STEREO_LEFT_RIGHT_HALF;
        }
        video_frame_template.stereo_layout_swap = (val == "right_left");
    }
    else if (val == "top_bottom" || val == "bottom_top")
    {
        if (video_frame_template.raw_size.height() > video_frame_template.raw_size.width())
        {
            video_frame_template.stereo_layout = STEREO_TOP_BOTTOM;
        }
        else
        {
            video_frame_template.stereo_layout = STEREO_TOP_BOTTOM_HALF;
        }
        video_frame_template.stereo_layout_swap = (val == "bottom_top");
    }
    else if (val == "row_interleaved_lr" || val == "row_interleaved_rl")
    {
        video_frame_template.stereo_layout = STEREO_EVEN_ODD_ROWS;
        video_frame_template.stereo_layout_swap = (val == "row_interleaved_rl");
    }
    else if (val == "block_lr" || val == "block_rl")
    {
        video_frame_template.stereo_layout = STEREO_ALTERNATING;
        video_frame_template.stereo_layout_swap = (val == "block_rl");
    }
    else if (!val.isEmpty())
    {
        qDebug() << QString("%1 video stream %2: Unsupported stereo layout %3.").
               arg(_url.toLatin1().constData()).
               arg(index + 1).
               arg(val.toLatin1().constData());
        video_frame_template.stereo_layout = STEREO_MONO;
        video_frame_template.stereo_layout_swap = false;
    }
    if (((video_frame_template.stereo_layout == STEREO_LEFT_RIGHT
            || video_frame_template.stereo_layout == STEREO_LEFT_RIGHT_HALF)
                && video_frame_template.raw_size.width() % 2 != 0)
            || ((video_frame_template.stereo_layout == STEREO_TOP_BOTTOM
            || video_frame_template.stereo_layout == STEREO_TOP_BOTTOM_HALF)
                && video_frame_template.raw_size.height() % 2 != 0)
            || (video_frame_template.stereo_layout == STEREO_EVEN_ODD_ROWS
                && video_frame_template.raw_size.height() % 2 != 0))
    {
        video_frame_template.stereo_layout = STEREO_MONO;
        video_frame_template.stereo_layout_swap = false;
    }

    video_frame_template.set_view_dimensions();
}

void media_object_p::set_video_template(int index,
                                            int width_avcodec,
                                            int height_avcodec)
{
    //处理模板帧的尺寸和纵横比
    frame_template_news(index, width_avcodec, height_avcodec);
    frame_template_color(index);//处理模板帧的数据布局和颜色空间
    frame_template_stereo(index);
}
void media_object_p::set_audio_template(int index)
{
    AVStream *audio_stream = _ffmpeg->f_ctx->streams[_ffmpeg->audio.streams[index]];
    AVCodecContext *audio_codec_ctx = _ffmpeg->audio.ctxs[index];
    audio_blob &audio_blob_template = _ffmpeg->audio.templates[index];

    AVDictionaryEntry *tag = av_dict_get(audio_stream->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX);
    if (tag)
    {
        audio_blob_template.language = tag->value;
    }
    if (audio_codec_ctx->channels < 1
            || audio_codec_ctx->channels > 8
            || audio_codec_ctx->channels == 3
            || audio_codec_ctx->channels == 5)
    {
        DEBUG( QString("%1 audio stream %2: Cannot handle audio with %3 channels.").
                arg(_url.toUtf8().constData()).arg(index + 1).arg(audio_codec_ctx->channels));
    }
    audio_blob_template.channels = audio_codec_ctx->channels;
    audio_blob_template.rate = audio_codec_ctx->sample_rate;
    if (audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_U8
        || audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_U8P)
    {
        audio_blob_template.sample_format = FORMAT_U8;
    }
    else if (audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16
            || audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_S16P)
    {
        audio_blob_template.sample_format = FORMAT_S16;
    }
    else if (audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_FLT
            || audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_FLTP)
    {
        audio_blob_template.sample_format = FORMAT_F32;
    }
    else if (audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_DBL
            || audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_DBLP)
    {
        audio_blob_template.sample_format = FORMAT_D64;
    }
    else if ((audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_S32
                || audio_codec_ctx->sample_fmt == AV_SAMPLE_FMT_S32P)
            && sizeof(qint32) == sizeof(float))
    {
        // we need to convert this to AV_SAMPLE_FMT_FLT after decoding
        audio_blob_template.sample_format = FORMAT_F32;
    }
    else
    {
        DEBUG( QString("%1 audio stream %2: Cannot handle audio with sample format %3.").
                arg(_url.toUtf8().constData()).arg( index + 1).
                arg(av_get_sample_fmt_name(audio_codec_ctx->sample_fmt)));
    }
}
void media_object_p::set_subtitle_template(int index)
{
    AVStream *subtitle_stream = _ffmpeg->f_ctx->streams[_ffmpeg->subtitle.streams[index]];
    subtitle_box &subtitle_box_template = _ffmpeg->subtitle.templates[index];

    AVDictionaryEntry *tag = av_dict_get(subtitle_stream->metadata, "language", NULL, AV_DICT_IGNORE_SUFFIX);
    if (tag)
    {
        subtitle_box_template.language = tag->value;
    }
}

//////////////////////////////////////////////////////////////////////////////
/// 打开输入媒体
AVInputFormat * media_object_p::device_open(const device_request &dev_request, AVDictionary **iparams)
{
    AVInputFormat *iformat = NULL;
    //通知ffmpeg设备类型
    switch (dev_request.device)
    {
    case DEVICE_FIREWIRE:
        iformat = av_find_input_format("libdc1394");
        break;
    case DEVICE_X11:
        iformat = av_find_input_format("x11grab");
        break;
    case DEVICE_SYS_DEFAULT:
#if (defined _WIN32 || defined __WIN32__) && !defined __CYGWIN__
        iformat = av_find_input_format("vfwcap");
#elif defined __FreeBSD__ || defined __NetBSD__ || defined __OpenBSD__ || defined __APPLE__
        iformat = av_find_input_format("bktr");
#else
        iformat = av_find_input_format("video4linux2");
#endif
        break;
    case DEVICE_NO:
        {
            QString extension = get_extension(_url);
            if (extension == "mpo" || extension == "jps")
                iformat = av_find_input_format("mjpeg");
        }
        break;
    }

    //若不支持当前设备格式，则抛出异常
    if (_is_device && !iformat)
    {
        DEBUG( QString("No support available for %1 device.").
                arg(
                dev_request.device == DEVICE_FIREWIRE ? ("Firewire")
                : dev_request.device == DEVICE_X11 ? ("X11")
                : ("default")));
    }

    //若指定了设备格式，并指定了设备的尺寸则对设备进行设置
    if (_is_device && dev_request.width != 0 && dev_request.height != 0)
    {
        av_dict_set(iparams, "video_size", QString("%1x%2").arg(dev_request.width)
                    .arg(dev_request.height).toUtf8().constData(), 0);
    }
    //若指定了设备格式，并指定了设备的帧速率则对设备进行设置
    if (_is_device && dev_request.frame_rate_num != 0 && dev_request.frame_rate_den != 0)
    {
        av_dict_set(iparams, "framerate", QString("%1/%2").arg(dev_request.frame_rate_num)
                    .arg(dev_request.frame_rate_den).toUtf8().constData(), 0);
    }
    //若指定了设备格式，并指定了设备的格式为mjpeg则对设备进行设置
    if (_is_device && dev_request.request_mjpeg)
    {
        av_dict_set(iparams, "input_format", "mjpeg", 0);
    }

    if (_is_device)
    {
        // 若为摄像设备则不启动预读取，容易产生启动延时
        _ffmpeg->f_ctx->max_analyze_duration = 0;
    }
    return iformat;
}
void media_object_p::decoder_open()
{
    int e = 0;
    //处理解码器并打开解码器
    for (uint i = 0; (i < _ffmpeg->f_ctx->nb_streams)
            && (i < static_cast<uint>(0xffffffff)); i++)
    {
        _ffmpeg->f_ctx->streams[i]->discard = AVDISCARD_ALL; // ignore by default; user must activate streams
        AVCodecContext *codec_ctx = _ffmpeg->f_ctx->streams[i]->codec;
        AVCodec *codec = (codec_ctx->codec_id == CODEC_ID_TEXT
                ? NULL : avcodec_find_decoder(codec_ctx->codec_id));

        int width_avcodec = codec_ctx->width;
        int height_avcodec = codec_ctx->height;
        if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            //得到系统处理器数，设置解码器线程数
            codec_ctx->thread_count = video_decoding_threads();
            if (codec_ctx->lowres || (codec && (codec->capabilities & CODEC_CAP_DR1)))
                codec_ctx->flags |= CODEC_FLAG_EMU_EDGE;
        }
        // 寻找解码器并打开
        if ((codec_ctx->codec_id != CODEC_ID_TEXT) &&
            ((!codec) || ((e = avcodec_open2(codec_ctx, codec, NULL)) < 0)))
        {
           qDebug() << QString("%1 stream %2: Cannot open %3: %4").arg( _url.toLatin1().constData()).
                  arg( i).
                  arg(
                      codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO
                      ? ("video codec")
                      : codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO
                      ? ("audio codec")
                      : codec_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE
                      ? ("subtitle codec")
                      : ("data")).
                  arg(codec ? my_av_strerror(e).toLatin1().constData()
                            : ("codec not supported"));
        }
        //视频流解码器
        else if (codec_ctx->codec_type == AVMEDIA_TYPE_VIDEO)
        {
            _ffmpeg->video.streams << (i);
            int j = _ffmpeg->video.streams.size() - 1;

            DEBUG(_url + " stream " + QString::number(i) + " is video stream " + QString::number(j) + ".");
            _ffmpeg->video.ctxs <<(codec_ctx);

            if (_ffmpeg->video.ctxs[j]->width < 1 ||
                    _ffmpeg->video.ctxs[j]->height < 1)
            {
                DEBUG( _url +QString(" video stream %1: Invalid frame size.").arg(j + 1));
            }
            _ffmpeg->video.codecs << (codec);

            // 设置模板帧
            _ffmpeg->video.templates <<(video_frame());
            set_video_template(j, width_avcodec, height_avcodec);

            // 分配解码时需要的空间
            _ffmpeg->video.packets << (AVPacket());
            av_init_packet(&(_ffmpeg->video.packets[j]));
            //分配解码线程
            _ffmpeg->video.threads << (new video_decode_thread(_url, _ffmpeg, j));
            _ffmpeg->video.frames << avcodec_alloc_frame();
            _ffmpeg->video.buffered_frames << avcodec_alloc_frame();

            enum PixelFormat frame_fmt =
                    (_ffmpeg->video.templates[j].layout == LAYOUT_BGRA32
                    ? PIX_FMT_BGRA
                    : _ffmpeg->video.ctxs[j]->pix_fmt);

            int frame_bufsize = (avpicture_get_size(frame_fmt,
                        _ffmpeg->video.ctxs[j]->width,
                        _ffmpeg->video.ctxs[j]->height));

            _ffmpeg->video.buffers << (static_cast<quint8 *>(av_malloc(frame_bufsize)));

            avpicture_fill(reinterpret_cast<AVPicture *>(_ffmpeg->video.buffered_frames[j]),
                           _ffmpeg->video.buffers[j],
                           frame_fmt,
                           _ffmpeg->video.ctxs[j]->width,
                           _ffmpeg->video.ctxs[j]->height);

            if (!_ffmpeg->video.frames[j] || !_ffmpeg->video.buffered_frames[j] || !_ffmpeg->video.buffers[j])
            {
                DEBUG( QString(HERE + ": " + strerror(ENOMEM)));
            }

            //若要转换到BGRA则分配转换需要的空间
            if (_ffmpeg->video.templates[j].layout == LAYOUT_BGRA32)
            {
                int sws_bufsize = avpicture_get_size(PIX_FMT_BGRA,
                        _ffmpeg->video.ctxs[j]->width,
                        _ffmpeg->video.ctxs[j]->height);

                _ffmpeg->video.sws_frames << avcodec_alloc_frame();

                _ffmpeg->video.sws_buffers <<
                        (static_cast<uint8_t *>(av_malloc(sws_bufsize)));

                if (!_ffmpeg->video.sws_frames[j] ||
                        !_ffmpeg->video.sws_buffers[j])
                {
                    DEBUG( QString(HERE + ": " + strerror(ENOMEM)));
                }
                avpicture_fill(reinterpret_cast<AVPicture *>
                               (_ffmpeg->video.sws_frames[j]),
                               _ffmpeg->video.sws_buffers[j],
                               PIX_FMT_BGRA,
                               _ffmpeg->video.ctxs[j]->width,
                               _ffmpeg->video.ctxs[j]->height);

                _ffmpeg->video.sws_ctxs << (sws_getCachedContext(NULL,
                            _ffmpeg->video.ctxs[j]->width,
                            _ffmpeg->video.ctxs[j]->height,
                            _ffmpeg->video.ctxs[j]->pix_fmt,
                            _ffmpeg->video.ctxs[j]->width,
                            _ffmpeg->video.ctxs[j]->height, PIX_FMT_BGRA,
                            SWS_POINT, NULL, NULL, NULL));

                if (!_ffmpeg->video.sws_ctxs[j])
                {
                    DEBUG( _url+QString(" video stream %1: Cannot initialize conversion context.").
                            arg(j + 1));
                }
            }
            else
            {
                _ffmpeg->video.sws_frames << (NULL);
                _ffmpeg->video.sws_buffers << (NULL);
                _ffmpeg->video.sws_ctxs << (NULL);
            }
            _ffmpeg->video.last_time << 0UL;
        }
        //音频流解码器
        else if (codec_ctx->codec_type == AVMEDIA_TYPE_AUDIO)
        {
            _ffmpeg->audio.streams << (i);
            int j = _ffmpeg->audio.streams.size() - 1;

            DEBUG(_url + " stream " + QString::number(i) + " is audio stream " + QString::number(j) + ".");
            _ffmpeg->audio.ctxs << (codec_ctx);
            _ffmpeg->audio.codecs << (codec);
            _ffmpeg->audio.templates << (audio_blob());

            set_audio_template(j);
            _ffmpeg->audio.threads << (new audio_decode_thread(_url, _ffmpeg, j));

            _ffmpeg->audio.tmpbufs << (static_cast<uchar*>(av_malloc(audio_tmpbuf_size)));
            if (!_ffmpeg->audio.tmpbufs[j])
            {
                DEBUG( QString(HERE + ": " + strerror(ENOMEM)));
            }
            _ffmpeg->audio.blobs << (blob());
            _ffmpeg->audio.buffers << (QVector<uchar>());
            _ffmpeg->audio.last_time << (0UL);
        }
        //字幕流解码器
        else if (codec_ctx->codec_type == AVMEDIA_TYPE_SUBTITLE)
        {
            _ffmpeg->subtitle.streams << (i);
            int j = _ffmpeg->subtitle.streams.size() - 1;
            DEBUG(_url + " stream " + QString::number(i) + " is subtitle stream " +QString::number(j)+ ".");

            _ffmpeg->subtitle.ctxs << (codec_ctx);

            _ffmpeg->subtitle.codecs << (
                    _ffmpeg->subtitle.ctxs[j]->codec_id == CODEC_ID_TEXT
                        ? NULL
                        : codec);

            _ffmpeg->subtitle.templates << (subtitle_box());
            set_subtitle_template(j);
            _ffmpeg->subtitle.threads << (new subtitle_decode_thread(_url, _ffmpeg, j));
            _ffmpeg->subtitle.box_buffers << (QQueue<subtitle_box>());
            _ffmpeg->subtitle.last_time << (0UL);
        }
        else
        {
            DEBUG(_url + " stream " + QString::number(i) +
                  " contains neither video nor audio nor subtitles.");
        }
    }
}
void media_object_p::dump_format()
{
    int e = avformat_find_stream_info(_ffmpeg->f_ctx, NULL);
    //获取输入流信息
    if (e < 0)
    {
        qDebug() << ( _url+": Cannot read stream info: "+my_av_strerror(e));
    }

    //打印当前流信息
    av_dump_format(_ffmpeg->f_ctx, 0, _url.toUtf8().constData(), 0);
}
void media_object_p::metadata()
{
    AVDictionaryEntry *tag = NULL;
    /*读取元数据 */
    while ((tag = av_dict_get(_ffmpeg->f_ctx->metadata, "", tag, AV_DICT_IGNORE_SUFFIX)))
    {
        _tag_names << (tag->key);
        _tag_values << (tag->value);
    }
}
void media_object_p::queues_alloc()
{
    //分配队列空间
    _ffmpeg->video.packet.resize(_ffmpeg->video.streams.size());
    _ffmpeg->audio.packet.resize(_ffmpeg->audio.streams.size());
    _ffmpeg->subtitle.packet.resize(_ffmpeg->subtitle.streams.size());

    //分配队列互斥空间并申请对象
    _ffmpeg->video.mutexes.resize(_ffmpeg->video.streams.size ());
    _ffmpeg->audio.mutexes.resize(_ffmpeg->audio.streams.size());
    _ffmpeg->subtitle.mutexes.resize(_ffmpeg->subtitle.streams.size());
}
void media_object::print_open_news()
{
    //打印当前输入媒体信息
    DEBUG(_dptr->_url + ":");
    for (int i = 0; i < video_streams(); i++)
    {
        DEBUG( QString("Video stream %1: %2 / %3, %4 seconds").arg( i)
               .arg(video_template(i).format_info().toLatin1().data())
               .arg(video_template(i).format_name().toLatin1().data())
               .arg(video_duration(i) / 1e6f));
        DEBUG(QString("Using up to %1 threads for decoding.")
               .arg(_dptr->_ffmpeg->video.ctxs.at(i)->thread_count));
    }
    for (int i = 0; i < audio_streams(); i++)
    {
        DEBUG(QString("Audio stream %1: %2 / %3, %4 seconds").arg(i)
               .arg(audio_template(i).format_info().toLatin1().data())
               .arg(audio_template(i).format_name().toLatin1().data())
               .arg(audio_duration(i) / 1e6f));
    }
    for (int i = 0; i < subtitle_streams(); i++)
    {
        DEBUG(QString("Subtitle stream %1: %2 / %3, %4 seconds").arg(i)
               .arg(subtitle_template(i).format_info().toLatin1().data())
               .arg(subtitle_template(i).format_name().toLatin1().data())
               .arg(subtitle_duration(i) / 1e6f));
    }
    if (video_streams() == 0 && audio_streams() == 0 && subtitle_streams() == 0)
    {
       DEBUG("No usable streams.");
    }

}

void media_object::open(const QString &url, const device_request &dev_request)
{
    Q_ASSERT(!_dptr->_ffmpeg);
    int e = 0;
    AVInputFormat *iformat = NULL;
    AVDictionary *iparams = NULL;

    _dptr->_url = url;
    _dptr->_is_device = dev_request.is_device();
    _dptr->_ffmpeg = new ffmpeg_stuff;
    _dptr->_ffmpeg->f_ctx = NULL;
    _dptr->_ffmpeg->have_active_audio_stream = false;
    _dptr->_ffmpeg->pos = 0;

    //创建读取线程
    _dptr->_ffmpeg->reader = new read_thread(_dptr->_url, _dptr->_is_device, _dptr->_ffmpeg);

    //若指定了设备进设置并返回设备输入格式；
    iformat = _dptr->device_open(dev_request, &iparams);
    /* 打开输入*/
    _dptr->_ffmpeg->f_ctx = NULL;
    if ((e = avformat_open_input(&_dptr->_ffmpeg->f_ctx,
                                 _dptr->_url.toUtf8().constData(),
                                 iformat, &iparams)) != 0
            || !_dptr->_ffmpeg->f_ctx)
    {
        av_dict_free(&iparams);
        DEBUG( QString("%1: %1").arg(_dptr->_url.toUtf8().constData()).
                arg(my_av_strerror(e).toUtf8().constData()));
        return ;
    }
    av_dict_free(&iparams);

    if (_dptr->_is_device)
    {
        _dptr->_ffmpeg->f_ctx->max_analyze_duration = 0;
    }
    _dptr->dump_format();//打印avformat信息
    _dptr->metadata();//获取avformat元数据

    _dptr->_ffmpeg->have_active_audio_stream = false;
    _dptr->_ffmpeg->pos = 0UL;
    _dptr->decoder_open();//查找解码器并打开解码器
    _dptr->queues_alloc();//分配队列空间
    print_open_news();//打印打开的信息
}

//////////////////////////////////////////////////////////////////////////////
/// 获取基本信息
const QString &media_object::url() const
{
    return _dptr->_url;
}
long media_object::tags() const
{
    return _dptr->_tag_names.size();
}
const QString &media_object::tag_name(long i) const
{
    Q_ASSERT(i < tags());
    return _dptr->_tag_names[i];
}
const QString &media_object::tag_value(long i) const
{
    Q_ASSERT(i < tags());
    return _dptr->_tag_values[i];
}
const QString &media_object::tag_value(const QString &tag_name) const
{
    return _dptr->tag_value(tag_name);
}
const QString &media_object_p::tag_value(const QString &tag_name) const
{
    static QString empty;
    for (int i = 0; i < _tag_names.size(); i++)
    {
        if (QString(tag_name) == _tag_names[i])
        {
            return _tag_values[i];
        }
    }
    return empty;
}

//////////////////////////////////////////////////////////////////////////////
/// 返回视频流索引
int media_object::video_streams() const
{
    return _dptr->_ffmpeg->video.streams.size();
}
int media_object::audio_streams() const
{
    return _dptr->_ffmpeg->audio.streams.size();
}
int media_object::subtitle_streams() const
{
    return _dptr->_ffmpeg->subtitle.streams.size();
}

//////////////////////////////////////////////////////////////////////////////
/// 设置激活的视频流
void media_object_p::stop_decode_thread_all()
{
    // 当激活指定视频流时，要停止所有解码线程
    for (int i = 0; i < _ffmpeg->video.streams.size(); i++)
    {
        _ffmpeg->video.threads[i]->finish();
    }
    for (int i = 0; i < _ffmpeg->audio.streams.size(); i++)
    {
        _ffmpeg->audio.threads[i]->finish();
    }
    for (int i = 0; i < _ffmpeg->subtitle.streams.size(); i++)
    {
        _ffmpeg->subtitle.threads[i]->finish();
    }
    // 当激活指定视频流时，要停止读取线程
    _ffmpeg->reader->finish();
}

void media_object::video_stream_set_active(int index, bool active)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < video_streams());

    _dptr->stop_decode_thread_all();

    // 设置激活状态
    _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->video.streams.at(index)]->discard =
        (active ? AVDISCARD_DEFAULT : AVDISCARD_ALL);

    // 重新启动读取线程
    _dptr->_ffmpeg->reader->start();
}
void media_object::audio_stream_set_active(int index, bool active)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < audio_streams());

    _dptr->stop_decode_thread_all();

    // Set status
    _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->audio.streams.at(index)]->discard =
        (active ? AVDISCARD_DEFAULT : AVDISCARD_ALL);
    _dptr->_ffmpeg->have_active_audio_stream = false;

    for (int i = 0; i < audio_streams(); i++)
    {
        if (_dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->audio.streams.at(index)]->discard
                == AVDISCARD_DEFAULT)
        {
            _dptr->_ffmpeg->have_active_audio_stream = true;
            break;
        }
    }

    // 重新启动读取线程
    _dptr->_ffmpeg->reader->start();
}
void media_object::subtitle_stream_set_active(int index, bool active)
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < subtitle_streams());

    _dptr->stop_decode_thread_all();

    // Set status
    _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->subtitle.streams.at(index)]->discard =
        (active ? AVDISCARD_DEFAULT : AVDISCARD_ALL);
    // 重新启动读取线程
    _dptr->_ffmpeg->reader->start();
}

//////////////////////////////////////////////////////////////////////////////
/// 返回指定模板帧数据
const video_frame &media_object::video_template(int video_stream) const
{
    Q_ASSERT(video_stream >= 0);
    Q_ASSERT(video_stream < video_streams());
    return _dptr->_ffmpeg->video.templates.at(video_stream);
}
const audio_blob &media_object::audio_template(int audio_stream) const
{
    Q_ASSERT(audio_stream >= 0);
    Q_ASSERT(audio_stream < audio_streams());
    return _dptr->_ffmpeg->audio.templates.at(audio_stream);
}
const subtitle_box &media_object::subtitle_template(int subtitle_stream) const
{
    Q_ASSERT(subtitle_stream >= 0);
    Q_ASSERT(subtitle_stream < subtitle_streams());
    return _dptr->_ffmpeg->subtitle.templates.at(subtitle_stream);
}

//////////////////////////////////////////////////////////////////////////////
/// 返回帧率和持续时间
int media_object::video_rate_numerator(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < video_streams());
    int n = _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->video.streams.at(index)]->r_frame_rate.num;
    if (n <= 0)
        n = 1;
    return n;
}
int media_object::video_rate_denominator(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < video_streams());
    int d = _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->video.streams.at(index)]->r_frame_rate.den;
    if (d <= 0)
        d = 1;
    return d;
}
qint64 media_object::video_duration(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < video_streams());
    return stream_duration(
            _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->video.streams.at(index)],
            _dptr->_ffmpeg->f_ctx);
}
qint64 media_object::audio_duration(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < audio_streams());
    return stream_duration(
            _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->audio.streams.at(index)],
            _dptr->_ffmpeg->f_ctx);
}
qint64 media_object::subtitle_duration(int index) const
{
    Q_ASSERT(index >= 0);
    Q_ASSERT(index < subtitle_streams());
    return stream_duration(
            _dptr->_ffmpeg->f_ctx->streams[_dptr->_ffmpeg->subtitle.streams.at(index)],
            _dptr->_ffmpeg->f_ctx);
}

//////////////////////////////////////////////////////////////////////////////
/// 启动解码
void media_object::start_video_read(int video_stream, int raw_frames)
{
    Q_ASSERT(video_stream >= 0);
    Q_ASSERT(video_stream < video_streams());
    Q_ASSERT(raw_frames == 1 || raw_frames == 2);
    _dptr->_ffmpeg->video.threads[video_stream]->set_raw_frames(raw_frames);
    _dptr->_ffmpeg->video.threads[video_stream]->start();
}
void media_object::start_audio_read(int audio_stream, int size)
{
    Q_ASSERT(audio_stream >= 0);
    Q_ASSERT(audio_stream < audio_streams());
    _dptr->_ffmpeg->audio.blobs[audio_stream].resize(size);
    _dptr->_ffmpeg->audio.threads[audio_stream]->start();
}
void media_object::start_subtitle_read(int subtitle_stream)
{
    Q_ASSERT(subtitle_stream >= 0);
    Q_ASSERT(subtitle_stream < subtitle_streams());
    _dptr->_ffmpeg->subtitle.threads[subtitle_stream]->start();
}

//////////////////////////////////////////////////////////////////////////////
/// 暂停解码并返回帧数据
video_frame media_object::finish_video_read(int video_stream)
{
    Q_ASSERT(video_stream >= 0);
    Q_ASSERT(video_stream < video_streams());
    _dptr->_ffmpeg->video.threads[video_stream]->finish();
    return _dptr->_ffmpeg->video.threads[video_stream]->frame();
}
audio_blob media_object::finish_audio_read(int audio_stream)
{
    Q_ASSERT(audio_stream >= 0);
    Q_ASSERT(audio_stream < audio_streams());
    _dptr->_ffmpeg->audio.threads[audio_stream]->finish();
    return _dptr->_ffmpeg->audio.threads[audio_stream]->blob();
}
subtitle_box media_object::finish_subtitle_read(int subtitle_stream)
{
    Q_ASSERT(subtitle_stream >= 0);
    Q_ASSERT(subtitle_stream < subtitle_streams());
    _dptr->_ffmpeg->subtitle.threads[subtitle_stream]->finish();
    return _dptr->_ffmpeg->subtitle.threads[subtitle_stream]->box();
}

//////////////////////////////////////////////////////////////////////////////
/// 返回当前位置
qint64 media_object::tell()
{
    return _dptr->_ffmpeg->pos;
}
bool media_object::eof()
{
    return _dptr->_ffmpeg->reader->eof();
}
//////////////////////////////////////////////////////////////////////////////
/// 时移位置
void media_object_p::clear_streams_queues()
{
    // 清除所有队列数据
    for (int i = 0; i < _ffmpeg->video.streams.size(); i++)
    {
        avcodec_flush_buffers(_ffmpeg->f_ctx->streams[_ffmpeg->video.streams[i]]->codec);
        for (int j = 0; j < _ffmpeg->video.packet[i].size(); j++)
        {
            av_free_packet(&_ffmpeg->video.packet[i][j]);
        }
        _ffmpeg->video.packet[i].clear();
    }
    for (int i = 0; i < _ffmpeg->audio.streams.size(); i++)
    {
        avcodec_flush_buffers(_ffmpeg->f_ctx->streams[_ffmpeg->audio.streams[i]]->codec);
        _ffmpeg->audio.buffers[i].clear();
        for (int j = 0; j < _ffmpeg->audio.packet[i].size(); j++)
        {
            av_free_packet(&_ffmpeg->audio.packet[i][j]);
        }
        _ffmpeg->audio.packet[i].clear();
    }
    for (int i = 0; i < _ffmpeg->subtitle.streams.size(); i++)
    {
        if (_ffmpeg->f_ctx->streams[_ffmpeg->subtitle.streams[i]]->codec->codec_id != CODEC_ID_TEXT)
        {
            // CODEC_ID_TEXT has no decoder, so we cannot flush its buffers
            avcodec_flush_buffers(_ffmpeg->f_ctx->streams[_ffmpeg->subtitle.streams[i]]->codec);
        }
        _ffmpeg->subtitle.box_buffers[i].clear();
        for (int j = 0; j < _ffmpeg->subtitle.packet[i].size(); j++)
        {
            av_free_packet(&_ffmpeg->subtitle.packet[i][j]);
        }
        _ffmpeg->subtitle.packet[i].clear();
    }

}
void media_object::seek(qint64 dest_pos)
{
   DEBUG(_dptr->_url + ": Seeking from " +
                QString::number(_dptr->_ffmpeg->pos / 1e6f) + " to " +
                QString::number(dest_pos / 1e6f) + ".");

    // 移动时要停止所有线程
    _dptr->stop_decode_thread_all();
    _dptr->clear_streams_queues();

    // 通知读取下一帧时必须更新位置
    for (int i = 0; i < _dptr->_ffmpeg->video.streams.size(); i++)
    {
        _dptr->_ffmpeg->video.last_time[i] = 0UL;
    }
    for (int i = 0; i < _dptr->_ffmpeg->audio.streams.size(); i++)
    {
        _dptr->_ffmpeg->audio.last_time[i] = 0UL;
    }
    for (int i = 0; i < _dptr->_ffmpeg->subtitle.streams.size(); i++)
    {
        _dptr->_ffmpeg->subtitle.last_time[i] = 0UL;
    }
    _dptr->_ffmpeg->pos = 0UL;

    // Seek
    int e = av_seek_frame(_dptr->_ffmpeg->f_ctx, -1,
            dest_pos * AV_TIME_BASE / 1000000,
            dest_pos < _dptr->_ffmpeg->pos ?  AVSEEK_FLAG_BACKWARD : 0);
    if (e < 0)
    {
        DEBUG(QString("%1: Seeking failed.").arg(_dptr-> _url.toLatin1().constData()));
    }

    // 重新启动读取线程
    _dptr->_ffmpeg->reader->reset();
    _dptr->_ffmpeg->reader->start();
}

void media_object::close()
{
    if (_dptr->_ffmpeg)
    {
        try
        {
            _dptr->stop_decode_thread_all();
        }
        catch (...)
        {
        }
        if (_dptr->_ffmpeg->f_ctx)
        {
            for (int i = 0; i < _dptr->_ffmpeg->video.frames.size(); i++)
            {
                av_free(_dptr->_ffmpeg->video.frames[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.buffered_frames.size(); i++)
            {
                av_free(_dptr->_ffmpeg->video.buffered_frames[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.buffers.size(); i++)
            {
                av_free(_dptr->_ffmpeg->video.buffers[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.sws_frames.size(); i++)
            {
                av_free(_dptr->_ffmpeg->video.sws_frames[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.sws_buffers.size(); i++)
            {
                av_free(_dptr->_ffmpeg->video.sws_buffers[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.ctxs.size(); i++)
            {
                if (i < _dptr->_ffmpeg->video.codecs.size() && _dptr->_ffmpeg->video.codecs[i])
                {
                    avcodec_close(_dptr->_ffmpeg->video.ctxs[i]);
                }
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.sws_ctxs.size(); i++)
            {
                sws_freeContext(_dptr->_ffmpeg->video.sws_ctxs[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.packet.size(); i++)
            {
                if (_dptr->_ffmpeg->video.packet[i].size() > 0)
                {
                    DEBUG(_dptr->_url + ": " + QString::number(_dptr->_ffmpeg->video.packet[i].size())
                            + " unprocessed packets in video stream " + QString::number(i));
                }
                for (int j = 0; j < _dptr->_ffmpeg->video.packet[i].size(); j++)
                {
                    av_free_packet(&_dptr->_ffmpeg->video.packet[i][j]);
                }
            }
            for (int i = 0; i < _dptr->_ffmpeg->video.packets.size(); i++)
            {
                av_free_packet(&(_dptr->_ffmpeg->video.packets[i]));
            }
            for (int i = 0; i < _dptr->_ffmpeg->audio.ctxs.size(); i++)
            {
                if (i < _dptr->_ffmpeg->audio.codecs.size() && _dptr->_ffmpeg->audio.codecs[i])
                {
                    avcodec_close(_dptr->_ffmpeg->audio.ctxs[i]);
                }
            }
            for (int i = 0; i < _dptr->_ffmpeg->audio.packet.size(); i++)
            {
                if (_dptr->_ffmpeg->audio.packet[i].size() > 0)
                {
                    DEBUG(_dptr->_url + ": " + QString::number(_dptr->_ffmpeg->audio.packet[i].size())
                            + " unprocessed packets in audio stream " + QString::number(i));
                }
                for (int j = 0; j < _dptr->_ffmpeg->audio.packet[i].size(); j++)
                {
                    av_free_packet(&_dptr->_ffmpeg->audio.packet[i][j]);
                }
            }
            for (int i = 0; i < _dptr->_ffmpeg->audio.tmpbufs.size(); i++)
            {
                av_free(_dptr->_ffmpeg->audio.tmpbufs[i]);
            }
            for (int i = 0; i < _dptr->_ffmpeg->subtitle.ctxs.size(); i++)
            {
                if (i < _dptr->_ffmpeg->subtitle.codecs.size() && _dptr->_ffmpeg->subtitle.codecs[i])
                {
                    avcodec_close(_dptr->_ffmpeg->subtitle.ctxs[i]);
                }
            }
            for (int i = 0; i < _dptr->_ffmpeg->subtitle.packet.size(); i++)
            {
                if (_dptr->_ffmpeg->subtitle.packet[i].size() > 0)
                {
                    DEBUG(_dptr->_url + ": " + QString::number(_dptr->_ffmpeg->subtitle.packet[i].size())
                            + " unprocessed packets in subtitle stream " + QString::number(i));
                }
                for (int j = 0; j < _dptr->_ffmpeg->subtitle.packet[i].size(); j++)
                {
                    av_free_packet(&_dptr->_ffmpeg->subtitle.packet[i][j]);
                }
            }
            avformat_close_input(&_dptr->_ffmpeg->f_ctx);
        }
        delete _dptr->_ffmpeg->reader;
        delete _dptr->_ffmpeg;
        _dptr->_ffmpeg = NULL;
    }
    _dptr->_url = "";
    _dptr->_is_device = false;
    _dptr->_tag_names.clear();
    _dptr->_tag_values.clear();
}
