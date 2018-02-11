#include "media_input.h"
#include <QStringList>

static QString basename(const QString &url)
{
    QFileInfo info(url);

    return info.baseName ();
}

class media_input_p
{
public :
    bool _is_device;                        // 是否是一个设备（如相机）
    QString _id;                            // ID 输入: URL0[/URL1[/URL2[...]]]
    QVector<media_object> _media_objects;   // 媒体对象组合成一个输入
    QVector<QString> _tag_names;            // 元数据：标记名称
    QVector<QString> _tag_values;           // 元数据：标签值

    QVector<QString> _video_stream_names;       // 可用的视频流的描述
    QVector<QString> _audio_stream_names;       // 可用的音频流的描述
    QVector<QString> _subtitle_stream_names;    // 可用的字幕流的描述

    bool _sup_stereo;      // 输入支持立体布局的 “separate_streams”？
    int _active_video_stream;                   // 目前激活的视频流。
    int _active_audio_stream;                   // 目前激活的音频流。
    int _active_subtitle_stream;                // 目前激活的字幕流。
    bool _have_active_video_read;               // 是否开始读取视频帧。
    bool _have_active_audio_read;               // 是否开始读取音频数据。
    bool _have_active_subtitle_read;            // 是否开始读取字幕。
    int _last_audio_data_size;                  // Size of last audio blob read

    qint64 _duration;                          // 总持续时间。

    video_frame _video_frame;                   // 当前活动的视频流的视频帧的模板。
    audio_blob _audio_blob;                     // 当前音频流音频数据模板。
    subtitle_box _subtitle_box;                 // 当前字幕流字幕模板。

    media_input_p() :
        _active_video_stream(-1),
        _active_audio_stream(-1),
        _active_subtitle_stream(-1),
        _have_active_video_read(false),
        _have_active_audio_read(false),
        _have_active_subtitle_read(false),
        _last_audio_data_size(0),
        _duration(-1)
    {
    }

    void metadata_ope()
    {
        // 用文件的名称构建id(没有扩展名)
        _id = basename(_media_objects[0].url());
        for (int i = 1; i < _media_objects.size(); i++)
        {
            _id += '/';
            _id += basename(_media_objects[i].url());
        }

        // 元数据
        for (int i = 0; i < _media_objects.size(); i++)
        {
            // Note that we may have multiple identical tag names in our metadata
            for (int j = 0; j < _media_objects[i].tags(); j++)
            {
                _tag_names << (_media_objects[i].tag_name(j));
                _tag_values << (_media_objects[i].tag_value(j));
            }
        }
    }
    void streams_news()
    {
        // 收集 streams 与 stream 名称
        for (int i = 0; i < _media_objects.size(); i++)
        {
            for (int j = 0; j < _media_objects[i].video_streams(); j++)
            {
                _video_stream_names << (_media_objects[i].video_template(j).format_info());
            }
        }
        if (_video_stream_names.size() > 1)
        {
            for (int i = 0; i < _video_stream_names.size(); i++)
            {
                _video_stream_names[i].insert(0, QString("#/%1:").arg(_video_stream_names.size()));
            }
        }

        for (int i = 0; i < _media_objects.size(); i++)
        {
            for (int j = 0; j < _media_objects[i].audio_streams(); j++)
            {
                _audio_stream_names << (_media_objects[i].audio_template(j).format_info());
            }
        }
        if (_audio_stream_names.size() > 1)
        {
            for (int i = 0; i < _audio_stream_names.size(); i++)
            {
                _audio_stream_names[i].insert(0,QString("#/%1:").arg(_audio_stream_names.size()));
            }
        }

        for (int i = 0; i < _media_objects.size(); i++)
        {
            for (int j = 0; j < _media_objects[i].subtitle_streams(); j++)
            {
                _subtitle_stream_names << (_media_objects[i].subtitle_template(j).format_info());
            }
        }
        if (_subtitle_stream_names.size() > 1)
        {
            for (int i = 0; i < _subtitle_stream_names.size(); i++)
            {
                _subtitle_stream_names[i].insert(0,QString("#/%1:").arg(_subtitle_stream_names.size()));
            }
        }

    }
    void set_duration()
    {
        // 设置时间信息
        _duration = 0xffffffff;
        for (int i = 0; i < _media_objects.size(); i++)
        {
            for (int j = 0; j < _media_objects[i].video_streams(); j++)
            {
                qint64 d = _media_objects[i].video_duration(j);
                if (d < _duration)
                {
                    _duration = d;
                }
            }
            for (int j = 0; j < _media_objects[i].audio_streams(); j++)
            {
                qint64 d = _media_objects[i].audio_duration(j);
                if (d < _duration)
                {
                    _duration = d;
                }
            }
            // Ignore subtitle stream duration; it seems unreliable and is not important anyway.
        }
    }
};
media_input::media_input() :
    _dptr(new media_input_p())
{
}

media_input::~media_input()
{
    delete _dptr;
}

void media_input::get_video_stream(int stream, int &object, int &object_video) const
{
    Q_ASSERT(stream < video_streams());

    int i = 0;
    while (_dptr->_media_objects[i].video_streams() < stream + 1)
    {
        stream -= _dptr->_media_objects[i].video_streams();
        i++;
    }
    object = i;
    object_video = stream;
}
void media_input::get_audio_stream(int stream, int &object, int &object_audio) const
{
    Q_ASSERT(stream < audio_streams());

    int i = 0;
    while (_dptr->_media_objects[i].audio_streams() < stream + 1)
    {
        stream -= _dptr->_media_objects[i].audio_streams();
        i++;
    }
    object = i;
    object_audio = stream;
}
void media_input::get_subtitle_stream(int stream, int &object, int &object_subtitle) const
{
    Q_ASSERT(stream < subtitle_streams());

    int i = 0;
    while (_dptr->_media_objects[i].subtitle_streams() < stream + 1)
    {
        stream -= _dptr->_media_objects[i].subtitle_streams();
        i++;
    }
    object = i;
    object_subtitle = stream;
}

void media_input::open(const QStringList &urls,
                       const device_request &dev_request)
{
    Q_ASSERT(urls.size() > 0);

    // 打开媒体对象
    _dptr->_is_device = dev_request.is_device();
    _dptr->_media_objects.resize(urls.size());
    for (int i = 0; i < urls.size(); i++)
        _dptr->_media_objects[i].open(urls[i], dev_request);


    _dptr->metadata_ope();
    _dptr->streams_news();
    _dptr->set_duration();

    // 在 3dtv.at 视频中跳过电影广告

    // 寻找立体对象并设置活动的视频流
    _dptr->_sup_stereo = false;
    if (video_streams() == 2)
    {
        int o0, o1, v0, v1;
        get_video_stream(0, o0, v0);
        get_video_stream(1, o1, v1);
        video_frame t0 = _dptr->_media_objects[o0].video_template(v0);
        video_frame t1 = _dptr->_media_objects[o1].video_template(v1);
        if (t0.size.width() == t1.size.width()
                && t0.size.height() == t1.size.height()
                && (t0.aspect_ratio <= t1.aspect_ratio && t0.aspect_ratio >= t1.aspect_ratio)
                && t0.layout == t1.layout
                && t0.color_space == t1.color_space
                && t0.value_range == t1.value_range
                && t0.chroma_location == t1.chroma_location)
        {
            _dptr->_sup_stereo = true;
        }
    }

    if (_dptr->_sup_stereo)
    {
        _dptr->_active_video_stream = 0;
        int o, s;
        get_video_stream(_dptr->_active_video_stream, o, s);
        _dptr->_video_frame = _dptr->_media_objects[o].video_template(s);
        _dptr->_video_frame.stereo_layout = STEREO_SEPARATE;
    }
    else if (video_streams() > 0)
    {
        _dptr->_active_video_stream = 0;
        int o, s;
        get_video_stream(_dptr->_active_video_stream, o, s);
        _dptr->_video_frame = _dptr->_media_objects[o].video_template(s);
    }
    else
    {
        _dptr->_active_video_stream = -1;
    }
    if (_dptr->_active_video_stream >= 0)
    {
        select_video_stream(_dptr->_active_video_stream);
    }

    // 设置活动的音频流
    _dptr->_active_audio_stream = (audio_streams() > 0 ? 0 : -1);
    if (_dptr->_active_audio_stream >= 0)
    {
        int o, s;
        get_audio_stream(_dptr->_active_audio_stream, o, s);
        _dptr->_audio_blob = _dptr->_media_objects[o].audio_template(s);
        select_audio_stream(_dptr->_active_audio_stream);
    }

    // 设置活动的字幕流
    _dptr->_active_subtitle_stream = -1;       // 默认没有字幕
}

int media_input::urls() const
{
    return _dptr->_media_objects.size();
}
const QString &media_input::url(int i) const
{
    return _dptr->_media_objects[i].url();
}
const QString &media_input::id() const
{
    return _dptr->_id;
}
bool media_input::is_device() const
{
    return _dptr->_is_device;
}

int media_input::tags() const
{
    return _dptr->_tag_names.size();
}
const QString &media_input::tag_name(int i) const
{
    Q_ASSERT(_dptr->_tag_names.size() > i);
    return _dptr->_tag_names[i];
}
const QString &media_input::tag_value(int i) const
{
    Q_ASSERT(_dptr->_tag_values.size() > i);
    return _dptr->_tag_values[i];
}
const QString &media_input::tag_value(const QString &tag_name) const
{
    static QString empty;
    for (int i = 0; i < _dptr->_tag_names.size(); i++)
    {
        if (QString(tag_name) == _dptr->_tag_names[i])
        {
            return _dptr->_tag_values[i];
        }
    }
    return empty;
}

const video_frame &media_input::video_template() const
{
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    return _dptr->_video_frame;
}
const audio_blob &media_input::audio_template() const
{
    Q_ASSERT(_dptr->_active_audio_stream >= 0);
    return _dptr->_audio_blob;
}
const subtitle_box &media_input::subtitle_template() const
{
    Q_ASSERT(_dptr->_active_subtitle_stream >= 0);
    return _dptr->_subtitle_box;
}

int media_input::video_rate_numerator() const
{
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    int o, s;
    get_video_stream(_dptr->_active_video_stream, o, s);
    return _dptr->_media_objects[o].video_rate_numerator(s);
}
int media_input::video_rate_denominator() const
{
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    int o, s;
    get_video_stream(_dptr->_active_video_stream, o, s);
    return _dptr->_media_objects[o].video_rate_denominator(s);
}
qint64 media_input::video_duration() const
{
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    return static_cast<qint64>(video_rate_denominator()) * 1000000 / video_rate_numerator();
}

bool media_input::stereo_supported(StereoLayout layout, bool) const
{
    if (video_streams() < 1)
    {
        return false;
    }
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    Q_ASSERT(_dptr->_active_video_stream < video_streams());
    int o, s;
    get_video_stream(_dptr->_active_video_stream, o, s);
    const video_frame &t = _dptr->_media_objects[o].video_template(s);
    bool supported = true;
    if (((layout == STEREO_LEFT_RIGHT || layout == STEREO_LEFT_RIGHT_HALF) && t.raw_size.width() % 2 != 0)
            || ((layout == STEREO_TOP_BOTTOM || layout == STEREO_TOP_BOTTOM_HALF) && t.raw_size.height() % 2 != 0)
            || (layout == STEREO_EVEN_ODD_ROWS && t.raw_size.height() % 2 != 0)
            || (layout == STEREO_SEPARATE && !_dptr->_sup_stereo))
    {
        supported = false;
    }
    return supported;
}
void media_input::have_active()
{
    if (_dptr->_have_active_video_read)
    {
        (void)finish_video_read();
    }
    if (_dptr->_have_active_audio_read)
    {
        (void)finish_audio_read();
    }
    if (_dptr->_have_active_subtitle_read)
    {
        (void)finish_subtitle_read();
    }
}
void media_input::set_stereo_layout(StereoLayout layout, bool swap)
{
    Q_ASSERT(stereo_supported(layout, swap));

    have_active();

    int o, s;
    get_video_stream(_dptr->_active_video_stream, o, s);
    const video_frame &t = _dptr->_media_objects[o].video_template(s);
    _dptr->_video_frame = t;
    _dptr->_video_frame.stereo_layout = layout;
    _dptr->_video_frame.stereo_layout_swap = swap;
    _dptr->_video_frame.set_view_dimensions();

    select_video_stream(_dptr->_active_video_stream);
    if (layout == STEREO_SEPARATE)
    {
        qint64 pos = _dptr->_media_objects[o].tell();
        if (pos > 0UL)
        {
            seek(pos);
        }
    }
}

void media_input::select_video_stream(int video_stream)
{
    have_active();
    Q_ASSERT(video_stream >= 0);
    Q_ASSERT(video_stream < video_streams());
    if (_dptr->_video_frame.stereo_layout == STEREO_SEPARATE)
    {
        _dptr->_active_video_stream = 0;
        for (int i = 0; i < _dptr->_media_objects.size(); i++)
        {
            for (int j = 0; j < _dptr->_media_objects[i].video_streams(); j++)
            {
                _dptr->_media_objects[i].video_stream_set_active(j, true);
            }
        }
    }
    else
    {
        _dptr->_active_video_stream = video_stream;
        int o, s;
        get_video_stream(_dptr->_active_video_stream, o, s);
        for (int i = 0; i < _dptr->_media_objects.size(); i++)
        {
            for (int j = 0; j <_dptr-> _media_objects[i].video_streams(); j++)
            {
                _dptr->_media_objects[i].video_stream_set_active(j, (i == static_cast<int>(o) && j == s));
            }
        }
    }
    // Re-set video frame template
    StereoLayout stereo_layout_bak = _dptr->_video_frame.stereo_layout;
    bool stereo_layout_swap_bak = _dptr->_video_frame.stereo_layout_swap;
    int o, s;
    get_video_stream(_dptr->_active_video_stream, o, s);
    _dptr->_video_frame = _dptr->_media_objects[o].video_template(s);
    _dptr->_video_frame.stereo_layout = stereo_layout_bak;
    _dptr->_video_frame.stereo_layout_swap = stereo_layout_swap_bak;
    _dptr->_video_frame.set_view_dimensions();
}
void media_input::select_audio_stream(int audio_stream)
{
    have_active();
    Q_ASSERT(audio_stream >= 0);
    Q_ASSERT(audio_stream < audio_streams());
    _dptr->_active_audio_stream = audio_stream;
    int o, s;
    get_audio_stream(_dptr->_active_audio_stream, o, s);
    for (int i = 0; i < _dptr->_media_objects.size(); i++)
    {
        for (int j = 0; j < _dptr->_media_objects[i].audio_streams(); j++)
        {
            _dptr->_media_objects[i].audio_stream_set_active(j, (i == static_cast<int>(o) && j == s));
        }
    }
    // Re-set audio blob template
    _dptr->_audio_blob = _dptr->_media_objects[o].audio_template(s);
}
void media_input::select_subtitle_stream(int subtitle_stream)
{
    have_active();
    Q_ASSERT(subtitle_stream >= -1);
    Q_ASSERT(subtitle_stream < subtitle_streams());
    _dptr->_active_subtitle_stream = subtitle_stream;
    int o = -1, s = -1;
    if (_dptr->_active_subtitle_stream >= 0)
    {
        get_subtitle_stream(_dptr->_active_subtitle_stream, o, s);
    }
    for (int i = 0; i < _dptr->_media_objects.size(); i++)
    {
        for (int j = 0; j < _dptr->_media_objects[i].subtitle_streams(); j++)
        {
            _dptr->_media_objects[i].subtitle_stream_set_active(j, (i == static_cast<int>(o) && j == s));
        }
    }
    // Re-set subtitle box template
    if (_dptr->_active_subtitle_stream >= 0)
        _dptr->_subtitle_box = _dptr->_media_objects[o].subtitle_template(s);
    else
        _dptr->_subtitle_box = subtitle_box();
}

void media_input::start_video_read()
{
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    if (_dptr->_have_active_video_read)
    {
        return;
    }
    if (_dptr->_video_frame.stereo_layout == STEREO_SEPARATE)
    {
        int o0, s0, o1, s1;
        get_video_stream(0, o0, s0);
        get_video_stream(1, o1, s1);
        _dptr->_media_objects[o0].start_video_read(s0, 1);
        _dptr->_media_objects[o1].start_video_read(s1, 1);
    }
    else
    {
        int o, s;
        get_video_stream(_dptr->_active_video_stream, o, s);
        _dptr->_media_objects[o].start_video_read(s,
                _dptr->_video_frame.stereo_layout == STEREO_ALTERNATING ? 2 : 1);
    }
    _dptr->_have_active_video_read = true;
}
void media_input::start_audio_read(int size)
{
    Q_ASSERT(_dptr->_active_audio_stream >= 0);
    if (_dptr->_have_active_audio_read)
    {
        return;
    }
    int o, s;
    get_audio_stream(_dptr->_active_audio_stream, o, s);
    _dptr->_media_objects[o].start_audio_read(s, size);
    _dptr->_last_audio_data_size = size;
    _dptr->_have_active_audio_read = true;
}
void media_input::start_subtitle_read()
{
    Q_ASSERT(_dptr->_active_subtitle_stream >= 0);
    if (_dptr->_have_active_subtitle_read)
    {
        return;
    }
    int o, s;
    get_subtitle_stream(_dptr->_active_subtitle_stream, o, s);
    _dptr->_media_objects[o].start_subtitle_read(s);
    _dptr->_have_active_subtitle_read = true;
}

video_frame media_input::finish_video_read()
{
    Q_ASSERT(_dptr->_active_video_stream >= 0);
    if (!_dptr->_have_active_video_read)
    {
        start_video_read();
    }
    video_frame frame;
    if (_dptr->_video_frame.stereo_layout == STEREO_SEPARATE)
    {
        int o0, s0, o1, s1;
        get_video_stream(0, o0, s0);
        get_video_stream(1, o1, s1);
        video_frame f0 = _dptr->_media_objects[o0].finish_video_read(s0);
        video_frame f1 = _dptr->_media_objects[o1].finish_video_read(s1);
        if (f0.is_valid() && f1.is_valid())
        {
            frame = _dptr->_video_frame;
            for (int p = 0; p < 3; p++)
            {
                frame.data[0][p] = f0.data[0][p];
                frame.data[1][p] = f1.data[0][p];
                frame.line_size[0][p] = f0.line_size[0][p];
                frame.line_size[1][p] = f1.line_size[0][p];
            }
            frame.p_time = f0.p_time;
        }
    }
    else
    {
        int o, s;
        get_video_stream(_dptr->_active_video_stream, o, s);
        video_frame f = _dptr->_media_objects[o].finish_video_read(s);
        if (f.is_valid())
        {
            frame = _dptr->_video_frame;
            for (int v = 0; v < 2; v++)
            {
                for (int p = 0; p < 3; p++)
                {
                    frame.data[v][p] = f.data[v][p];
                    frame.line_size[v][p] = f.line_size[v][p];
                }
            }
            frame.p_time = f.p_time;
        }
    }
    _dptr->_have_active_video_read = false;
    return frame;
}
audio_blob media_input::finish_audio_read()
{
    Q_ASSERT(_dptr->_active_audio_stream >= 0);
    int o, s;
    get_audio_stream(_dptr->_active_audio_stream, o, s);
    if (!_dptr->_have_active_audio_read)
    {
        start_audio_read(_dptr->_last_audio_data_size);
    }
    _dptr->_have_active_audio_read = false;
    return _dptr->_media_objects[o].finish_audio_read(s);
}
subtitle_box media_input::finish_subtitle_read()
{
    Q_ASSERT(_dptr->_active_subtitle_stream >= 0);
    int o, s;
    get_subtitle_stream(_dptr->_active_subtitle_stream, o, s);
    if (!_dptr->_have_active_subtitle_read)
    {
        start_subtitle_read();
    }
    _dptr->_have_active_subtitle_read = false;
    return _dptr->_media_objects[o].finish_subtitle_read(s);
}

qint64 media_input::tell()
{
    qint64 pos = 0UL;
    int o, s;
    if (_dptr->_active_audio_stream >= 0)
    {
        get_audio_stream(_dptr->_active_audio_stream, o, s);
        pos = _dptr->_media_objects[o].tell();
    }
    else if (_dptr->_active_video_stream >= 0)
    {
        get_video_stream(_dptr->_active_video_stream, o, s);
        pos = _dptr->_media_objects[o].tell();
    }
    return pos;
}
bool media_input::eof()
{
    int o, s;
    if (_dptr->_active_audio_stream >= 0)
    {
        get_audio_stream(_dptr->_active_audio_stream, o, s);
        return _dptr->_media_objects[o].eof();
    }
    else if (_dptr->_active_video_stream >= 0)
    {
        get_video_stream(_dptr->_active_video_stream, o, s);
        return _dptr->_media_objects[o].eof();
    }
    return false;
}
void media_input::seek(qint64 pos)
{
    have_active();
    for (int i = 0; i < _dptr->_media_objects.size(); i++)
    {
        _dptr->_media_objects[i].seek(pos);
    }
}

void media_input::close()
{
    try
    {
        have_active();
        for (int i = 0; i < _dptr->_media_objects.size(); i++)
        {
            _dptr->_media_objects[i].close();
        }
    }
    catch (...)
    {
    }
    _dptr->_is_device = false;
    _dptr->_id = "";
    _dptr->_media_objects.clear();
    _dptr->_tag_names.clear();
    _dptr->_tag_values.clear();
    _dptr->_video_stream_names.clear();
    _dptr->_audio_stream_names.clear();
    _dptr->_subtitle_stream_names.clear();
    _dptr->_active_video_stream = -1;
    _dptr->_active_audio_stream = -1;
    _dptr->_active_subtitle_stream = -1;
    _dptr->_duration = -1;
    _dptr->_video_frame = video_frame();
    _dptr->_audio_blob = audio_blob();
    _dptr->_subtitle_box = subtitle_box();
}

int media_input::selected_video_stream() const
{
    return _dptr->_active_video_stream;
}
int media_input::selected_audio_stream() const
{
    return _dptr->_active_audio_stream;
}
int media_input::selected_subtitle_stream() const
{
    return _dptr->_active_subtitle_stream;
}
// 输入视频流数。
int media_input::video_streams() const
{
    return _dptr->_video_stream_names.size();
}

// 输入的音频流的数目。
int media_input::audio_streams() const
{
    return _dptr->_audio_stream_names.size();
}

// 输入字幕流数目。
int media_input::subtitle_streams() const
{
    return _dptr->_subtitle_stream_names.size();
}

// 获取给定视频流的名字。
const QString &media_input::video_stream_name(int video_stream) const
{
    return _dptr->_video_stream_names[video_stream];
}

// 获取给定音频流的名字。
const QString &media_input::audio_stream_name(int audio_stream) const
{
    return _dptr->_audio_stream_names[audio_stream];
}

// 获取给定字幕流的名字。
const QString &media_input::subtitle_stream_name(int subtitle_stream) const
{
    return _dptr->_subtitle_stream_names[subtitle_stream];
}


// 总持续时间。
qint64 media_input::duration() const
{
    return _dptr->_duration;
}
