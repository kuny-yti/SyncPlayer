#include "player.h"
#include "media_input.h"
#include <QDebug>
#include <QTimer>
#include <QImage>
#include <QThread>
#include <QDateTime>
#include <QDir>

class player_thread;
namespace impl {

class players
{
public:
    float _volume;
    bool _mute;
    eLoopMode _loop;
    update_ts _update_ts;
    qint64 _audio_delay;
    bool _benchmark;
    bool _is_open;

    media_input *_media_input;
    player_thread *_player_thread;
    audio_output *_audio_output;
    video_output *_video_output;

    video_frame _video_frame;
    subtitle_box _current_subtitle;
    subtitle_box _next_subtitle;

    float _frame_rate;
    qint64 _frame_count;
    //基准模式
    int _frames_shown;          // 显示自上次被重置的时间
    qint64 _fps_mark_time;     // 当 _frames_shown 的时间被重置为0

    qint64 _pause_start;       // 在暂停模式时间
    qint64 _current_pos;       // 当前输入的位置
    qint64 _start_pos;         // 初始输入的位置
    qint64 _video_pos;         // 当前视频帧的播放时间
    qint64 _audio_pos;         // 当前的音频数据播放时间
    qint64 _master_time_start; // 掌握时间偏移
    qint64 _master_time_current; // 目前掌握的时间
    qint64 _master_time_pos;     // 输入位置在开始主时间位置

    //播放状态
    bool _running;                              // 标志是否正在运行
    bool _first_frame;                          // 已经处理了第一帧
    bool _need_frame_now;                       // 现在需要另一个视频帧
    bool _need_frame_soon;                      // 立刻需要另一个视频帧
    bool _drop_next_frame;                      // 是否删除下一帧，让其同步
    bool _previous_frame_dropped;               // 是否有放弃以前的视频帧
    bool _recently_seeked;                      // 还没有显示最后一个视频帧
    bool _in_pause;                             // 是否在暂停模式下
    bool _play_end;

    //控制请求
    bool _quit_request;                         // 退出请求
    bool _pause_request;                        // 暂停请求
    bool _step_request;                         // 进入单帧步进
    qint64 _seek_request;       // 请求移动位置(相对于当前位置)
    double _set_pos_request;      // 请求移动的位置，绝对位置 (范围 0..1)

    QStringList _playlist;
    QString _curren_play_file;
    mutex _mutex_ope;
    player *_this_;

    qint64 step(bool *more_steps, bool *prep_frame, bool *drop_frame, bool *display_frame);

    void set_current_subtitle()
    {
        _current_subtitle = subtitle_box();
        if (_next_subtitle.is_valid()
                && _next_subtitle.p_start_time
                < _video_pos + _media_input->video_duration())
        {
            _current_subtitle = _next_subtitle;
        }
    }

    void reset_state()
    {
        _frame_count = 0;
        _audio_delay = 0;
        _benchmark = false;
        _running = false;
        _first_frame = false;
        _need_frame_now = false;
        _need_frame_soon = false;
        _drop_next_frame = false;
        _previous_frame_dropped = false;
        _in_pause = false;
        _recently_seeked = false;
        _quit_request = false;
        _pause_request = false;
        _step_request = false;
        _seek_request = 0;
        _set_pos_request = -1.0f;
        _video_frame = video_frame();
        _current_subtitle = subtitle_box();
        _next_subtitle = subtitle_box();
    }
    // 规范输入位置为[0,1]
    double normalize_pos(qint64 pos) const
    {
        if (_media_input->duration() > pos && pos >= 0)
            return double(pos) / double( _media_input->duration());
        else
            return 0.0f;
    }
};
}
class player_thread :public thread
{
public:
    player *_player_;
    bool abort;
    friend class player;

    player_thread(player *pay):
        _player_(pay)
    {
        abort = false;
    }

    ~player_thread()
    {
        abort = true;
        _player_->play(false);
    }

    void run()
    {
        while (_player_ && !abort)
            _player_->run_step();
    }
};

player::player(video_output *vo, audio_output *ao)
{
    impl = new impl::players();
    impl->_this_ = this;
    impl->_player_thread = new player_thread(this);
    impl->_audio_output = ao;
    impl->_video_output =  vo;

    impl->reset_state();
    impl->_media_input = 0;
    impl->_start_pos = 0UL;
    impl->_running = false;
    impl->_volume = 1.0;
    impl->_update_ts = NULL;
    impl->_mute = false;

    QDir dir; dir.remove ("logs.log");
}

player::~player()
{
    if (is_play ())
        play(false);
    if (impl->_player_thread)
        delete impl->_player_thread;
    if (impl->_media_input)
        delete impl->_media_input;
    delete impl;
}

//![0]
void player::playlist(const QStringList &list_path,  bool clear)
{
    if (clear)
    {
        impl->_playlist.clear ();
        impl->_playlist = list_path;
        impl->_curren_play_file = impl->_playlist[impl->_playlist.size ()-1];
    }
    else
        impl->_playlist += list_path;
}//设置播放器列表

QStringList player::playlist()
{
    return impl->_playlist;
}//返回播放器列表

void player::curren_file(const QString &file)
{
    impl->_curren_play_file = file;
    foreach (QString val, impl->_playlist)
    {
        if (file == val)
            return;
    }
    impl->_playlist << file;
}//设置当前播放的文件
QString player::curren_file()
{
    return impl->_curren_play_file;
}//当前播放的文件
//![0]

//![1]
void player::open(const QString&file)
{
    logs("open:"+file);
    if (impl->_is_open && file == curren_file ())
        return ;
    if (impl->_media_input || is_play())
        play(false);

    impl->reset_state();
    if (!file.isEmpty ())
        curren_file(file);

    if (!impl->_media_input)
        impl->_media_input = new media_input();

    if (curren_file().isEmpty())
        return ;

    QStringList list; list << curren_file();
    impl->_media_input->open(list);

    impl->_frame_rate = (float)impl->_media_input->video_rate_numerator () /
            (float)impl->_media_input->video_rate_denominator ();
    impl->_is_open = true;
    logs("open ok");

}//打开播放器
void player::close()
{
    impl->reset_state();
    if (impl->_media_input)
    {
        impl->_media_input->close();
        delete impl->_media_input;
        impl->_media_input = 0;
    }
    impl->_is_open = false;
    logs("close player");
}//关闭播放器
//![1]

//![2]
void player::play(bool s)
{
    if (!impl->_is_open)return ;
    if (s)
    {
        if (curren_file().isEmpty())
            return ;

        if (is_play() || !is_open())
        {
            impl->reset_state();
            if (!impl->_media_input)
                open(curren_file());
        }

        impl->_audio_output->deinit();
        impl->_audio_output->init();
        impl->_audio_output->unpause();

        impl->_play_end = false;
        impl->_player_thread->abort = false;
        impl->_player_thread->start();
        impl->_video_output->play_state(true);
        logs("player play");
    }
    else
    {
        impl->_video_output->play_state(false);
        impl->_player_thread->abort = true;
        impl->_player_thread->finish();
        close();
        logs("player stop");
    }
}//开始播放
void player::pause(bool s)
{
     impl->_pause_request = s;
     logs(QString("player ")+(s ? "pause" : "unpause"));
}//暂停

void player::seek(double val)
{
    impl->_set_pos_request = val;
    logs("player seek "+QString::number (val));
}//移动位置
void player::seek(qint64 val)
{
    impl->_seek_request = val;
    //impl->_media_input->seek (val);
    logs("player seek "+QString::number (val));
}

//![2]

//![3]
void player::mute(bool val)
{
    //ok
    if (val)
    {
        float bv = volume();
        volume(0.0);
        impl->_volume = bv;
        logs("player mute");
    }
    else
    {
        volume(volume());
        logs("player unmute");
    }
    impl->_mute = val;
}//静音
void player::volume(float val)
{
    //ok
    impl->_volume = val;
    impl->_audio_output->set_volume(val);
    logs("player volume:"+QString::number (val));
}//设置音量
float player::volume()
{
    //ok
    return impl->_volume;
}//返回当前音量
//![3]

//![3]
QImage player::capture()
{
    return QImage();
} //截图

//![3]
qint64 player::duration()const
{
    return impl->_media_input->duration();
}
double player::pos()const
{
    return impl->normalize_pos(impl->_current_pos);
}
qint64 player::tell()const
{
    return impl->_current_pos;
}
qint64 player::frame()const
{
    return impl->_frame_count;
}
float player::frame_rate()const
{
    return impl->_frame_rate;
}

void player::set(update_ts fun)
{
    impl->_update_ts = fun;
}
void player::loop(eLoopMode m)
{
    impl->_loop = m;
}

//![4]
bool player::is_mute()
{
    return impl->_mute;
} //是否静音
bool player::is_play()
{
    return impl->_running && !impl->_play_end;
} //是否播放
bool player::is_pause()
{
    return impl->_in_pause;
}//是否暂停
bool player::is_open()
{
    return impl->_is_open;
}
//![4]

void player::toggle_play()
{
    if (is_play())
        play(false);
    else
        play(true);
}//若在播放状态则停止
void player::toggle_pause()
{
    if (is_pause())
        pause(false);
    else
        pause(true);
}//若在播放状态则暂停
void player::toggle_mute()
{
    if (is_mute())
        mute(false);
    else
        mute(true);
}//若是静音则设置开启

qint64 impl::players::step(bool *more_steps,bool *prep_frame, bool *drop_frame, bool *display_frame)
{
    qint64 seek_to = -1;
    *more_steps = false; //更多的步骤
    *prep_frame = false; //准备帧
    *drop_frame = false; //丢帧
    *display_frame = false;//显示帧

    if (_quit_request)
    {
        _running = false;
        _this_->logs("quit_request");
        return 0;
    }
    else if (!_running)
    {
        _play_end = false;
        _this_->logs ("running");
        _media_input->start_video_read();
        _video_frame = _media_input->finish_video_read();
        if (!_video_frame.is_valid())
        {
            DEBUG("Empty video input.");
            return 0;
        }
        _video_pos = _video_frame.p_time;
        if (_media_input->selected_subtitle_stream() >= 0)
        {
            do
            {
                _media_input->start_subtitle_read();
                _next_subtitle = _media_input->finish_subtitle_read();
                if (!_next_subtitle.is_valid())
                {
                    DEBUG("Empty subtitle stream.");
                    return 0;
                }
            }
            while (_next_subtitle.p_stop_time < _video_pos);
        }

        if (_audio_output && (_media_input->selected_audio_stream() >= 0))
        {
            _media_input->start_audio_read(_audio_output->initial_size());
            audio_blob blob = _media_input->finish_audio_read();
            if (!blob.is_valid())
            {
                DEBUG("Empty audio input.");
                return 0;
            }
            _audio_pos = blob.p_time;
            try {
                _audio_output->data(blob);
            } catch (const QString &str) {
                _this_->logs("audio_data try: "+str);
                qDebug() << str;
            }

            _media_input->start_audio_read(_audio_output->update_size());
            _master_time_start = _audio_output->start();
            _master_time_pos = _audio_pos;
            _current_pos = _audio_pos;
        }
        else
        {
            _master_time_start = get_microseconds(TIMER_MONOTONIC);
            _master_time_pos = _video_pos;
            _current_pos = _video_pos;
        }

        _start_pos = _current_pos;
        _fps_mark_time = get_microseconds(TIMER_MONOTONIC);
        _frames_shown = 0;
        _running = true;
        _this_->logs ("running ok");

        {
            _need_frame_now = false;
            _need_frame_soon = true;
            _first_frame = true;
            *more_steps = true;
            *prep_frame = true;
            set_current_subtitle();
            return 0;
        }
    }
    if (_seek_request != 0 || _set_pos_request >= 0.0)
    {
        if (_set_pos_request >= 0.0)
        {
            _this_->logs("seek: "+QString::number (_set_pos_request));
            if (_media_input->duration() <= _start_pos)
                seek_to = _current_pos;
            else
                seek_to = double(_set_pos_request) * double(_media_input->duration());
        }
        else
        {
            if (_media_input->duration() > _seek_request && _seek_request > 0)
            {
                seek_to = _seek_request;
                _this_->logs("seek_to: "+QString::number (seek_to));
            }
        }
        _seek_request = 0;
        _set_pos_request = -1.0f;
        _media_input->seek(seek_to);
        _next_subtitle = subtitle_box();
        _current_subtitle = subtitle_box();

        _media_input->start_video_read();
        _video_frame = _media_input->finish_video_read();
        if (!_video_frame.is_valid())
        {
           DEBUG("Seeked to end of video?!");
            return 0;
        }
        _video_pos = _video_frame.p_time;

        if (_media_input->selected_subtitle_stream() >= 0)
        {
            do
            {
                _media_input->start_subtitle_read();
                _next_subtitle = _media_input->finish_subtitle_read();
                if (!_next_subtitle.is_valid())
                {
                    DEBUG("Seeked to end of subtitle?!");
                    return 0;
                }
            }
            while (_next_subtitle.p_stop_time < _video_pos);
        }
        if (_audio_output && (_media_input->selected_audio_stream() >= 0))
        {
            _audio_output->stop();
            _media_input->start_audio_read(_audio_output->initial_size());
            audio_blob blob = _media_input->finish_audio_read();
            if (!blob.is_valid())
            {
                DEBUG("Seeked to end of audio?!");
                return 0;
            }
            _audio_pos = blob.p_time;
            try {
                _audio_output->data(blob);
            } catch (const QString &str) {
                _this_->logs("audio_data try: "+str);
            }
            _media_input->start_audio_read(_audio_output->update_size());
            _master_time_start = _audio_output->start();
            _master_time_pos = _audio_pos;
            _current_pos = _audio_pos;
        }
        else
        {
            _master_time_start = get_microseconds(TIMER_MONOTONIC);
            _master_time_pos = _video_pos;
            _current_pos = _video_pos;
        }

        if (_update_ts && _current_pos > 0)
        {
            _update_ts(_current_pos, normalize_pos(_current_pos));

        }

        set_current_subtitle();
        _recently_seeked = true;
        *prep_frame = true;
        *more_steps = true;
        return 0;
    }
    else if (_recently_seeked)
    {
        _recently_seeked = false;
        _need_frame_now = true;
        _need_frame_soon = false;
        _previous_frame_dropped = false;
        *display_frame = true;
        *more_steps = true;
        return 0;
    }
    else if (_pause_request)
    {
        if (!_in_pause)
        {
            _this_->logs("pause_request");
            if (_audio_output && _media_input->selected_audio_stream() >= 0)
            {
                _audio_output->pause();
            }
            else
            {
                _pause_start = get_microseconds(TIMER_MONOTONIC);
            }
            _in_pause = true;

            if (_audio_output)
                _audio_output->pause();
        }
        *more_steps = true;
        return 1000;    // allow some sleep in pause mode
    }
    else if (_need_frame_now)
    {
        _video_frame = _media_input->finish_video_read();
        if (!_video_frame.is_valid())
        {
            if (_first_frame)
            {
                DEBUG("Single-frame video input: going into pause mode.");
                _pause_request = true;
            }
            else
            {
                DEBUG("End of video stream.");
                 _play_end = _media_input->eof();
                if (_loop == LOOP_CURRENT_FILE)
                {
                    _this_->logs("loop current");
                    _set_pos_request = 0.0f;
                    _play_end = false;
                    if (_update_ts)
                        _update_ts(0, 0.0);
                    *more_steps = true;
                }
                return 0;
            }
        }
        else
        {
            _first_frame = false;
        }

        _video_pos = _video_frame.p_time;
        if (_media_input->selected_subtitle_stream() >= 0)
        {
            while (_next_subtitle.is_valid()
                    && _next_subtitle.p_stop_time < _video_pos)
            {
                _next_subtitle = _media_input->finish_subtitle_read();
                _media_input->start_subtitle_read();
            }
        }
        if (!_audio_output
                || _media_input->selected_audio_stream() < 0)
        {
            _master_time_start += (_video_pos - _master_time_pos);
            _master_time_pos = _video_pos;
            _current_pos = _video_pos;

            if (_update_ts&& _current_pos > 0)
            {
                _update_ts(_current_pos, normalize_pos(_current_pos));
            }
        }
        _need_frame_now = false;
        _need_frame_soon = true;
        if (_drop_next_frame)
        {
            *drop_frame = true;
        }
        else if (!_pause_request)
        {
            *prep_frame = true;
            set_current_subtitle();
        }
        *more_steps = true;
        return 0;
    }
    else if (_need_frame_soon)
    {
        _media_input->start_video_read();
        _need_frame_soon = false;
        *more_steps = true;
        return 0;
    }
    else
    {
        if (_in_pause)
        {
            if (_audio_output
                && _media_input->selected_audio_stream() >= 0)
            {
                _audio_output->unpause();
            }
            else
            {
                _master_time_start += get_microseconds(TIMER_MONOTONIC) - _pause_start;
            }
           _in_pause = false;
           if (_audio_output)
               _audio_output->unpause();
        }
        if (_audio_output && _media_input->selected_audio_stream() >= 0)
        {
            bool need_audio_data;
            try {
            _master_time_current = _audio_output->status(&need_audio_data) -
                    _master_time_start + _master_time_pos;
            }
            catch (const QString &str)
            {
                _this_->logs("audio_state try:" +str);
            }

            if (need_audio_data)
            {
                audio_blob blob = _media_input->finish_audio_read();
                if (!blob.is_valid())
                {
                    DEBUG("End of audio stream.");
                    _play_end = _media_input->eof();
                    if (_loop == LOOP_CURRENT_FILE)
                    {
                        _this_->logs("loop current");
                        _set_pos_request = 0.0f;
                        _play_end = false;
                        if (_update_ts)
                            _update_ts(0, 0.0);
                        *more_steps = true;
                    }
                    return 0;
                }
                if (blob.size == _audio_output->update_size())
                {
                    _audio_pos = blob.p_time;
                    _master_time_start += (_audio_pos - _master_time_pos);
                    _master_time_pos = _audio_pos;
                    try {
                        _audio_output->data(blob);
                    } catch (const QString &str) {
                        _this_->logs("audio_data try:"+str);
                    }
                    _media_input->start_audio_read(_audio_output->update_size());
                    _current_pos = _audio_pos;
                    if (_update_ts && _current_pos > 0)
                    {
                        _update_ts(_current_pos, normalize_pos(_current_pos));
                    }
                }
            }
        }
        else
        {
            _master_time_current = get_microseconds(TIMER_MONOTONIC) - _master_time_start
                + _master_time_pos;
        }
        qint64 allowable_sleep = 0;
        qint64 next_frame_presentation_time = _master_time_current + _audio_delay;
        //if (_video_output)
            //next_frame_presentation_time += _video_output->time_to_next_frame_presentation();
        if (next_frame_presentation_time >= _video_pos || _benchmark || _media_input->is_device())
        {
            _drop_next_frame = false; // 设置丢帧
            qint64 delay = next_frame_presentation_time - _video_pos;
            if (delay > _media_input->video_duration() * 75 / 100
                    && !_benchmark
                    && !_media_input->is_device()
                    && !_step_request)
            {
                DEBUG(QString("Video: delay %1 seconds/%2 frames; dropping next frame.")
                            .arg(float(delay) / 1e6f)
                            .arg(float(delay) / float(_media_input->video_duration())));
                _drop_next_frame = true;// 设置丢帧
            }
            if (!_previous_frame_dropped)
            {
                *display_frame = true;
                if (_step_request)
                    _pause_request = true;
                if (_benchmark)
                {
                    _frames_shown++;
                    if (_frames_shown == 100)   //show fps each 100 frames
                    {
                        qint64 now = get_microseconds(TIMER_MONOTONIC);
                        DEBUG(QString("FPS: %1")
                              .arg(static_cast<float>(_frames_shown) /
                                   ((now - _fps_mark_time) / 1e6f)));
                        _fps_mark_time = now;
                        _frames_shown = 0;
                    }
                }
            }
            _need_frame_now = true;
            _need_frame_soon = false;
            _previous_frame_dropped = _drop_next_frame;// 设置丢帧
        }
        else
        {
            allowable_sleep = _video_pos - next_frame_presentation_time;
            if (allowable_sleep < 100)
            {
                allowable_sleep = 0;
            }
            else if (allowable_sleep > 1100)
            {
                allowable_sleep = 1100;
            }
            allowable_sleep -= 100;
        }
        *more_steps = true;
        return allowable_sleep;
    }
}

bool player::run_step()
{
    bool more_steps;//更多的步骤(标示是否是执行准备帧还是丢帧、显示帧)
    bool prep_frame;
    bool drop_frame;
    bool display_frame;
    qint64 allowed_sleep;

    allowed_sleep = impl->step(&more_steps, &prep_frame, &drop_frame, &display_frame);
    if (allowed_sleep > 2000)
        allowed_sleep = 1000;
    if (impl->_play_end)
    {
        impl->_player_thread->abort = true;
        impl->_set_pos_request = 0.0;
        impl->_in_pause = false;
        if (impl->_update_ts)
            impl->_update_ts(-2,0.0);
        logs("play end.");
    }
    if (!more_steps)//没有多余的步骤时返回
    {
        return false;
    }
    if (prep_frame)//有多余的步骤，执行准备帧
    {
        //if (impl->_current_subtitle_box.is_valid() && impl->_video_output)
        {
            //impl->_master_time_start += _video_output->wait_for_subtitle_renderer();
        }
        impl->_frame_count++;
        impl->_video_output->prepare_next_frame(impl->_video_frame, impl->_current_subtitle);
    }
    else if (drop_frame)//有多余的步骤，执行丢弃帧
    {
    }
    else if (display_frame)//有多余的步骤，执行显示帧
    {
        //impl->_video_output->activate_next_frame();
    }

    if (allowed_sleep > 0)
    {
        QThread::usleep(allowed_sleep);
    }

    return true;
}

void player::lock()
{
    impl->_mutex_ope.lock ();
}
void player::unlock()
{
    impl->_mutex_ope.unlock ();
}
void player::logs(const QString &text)
{
    static QFile logs_file("logs.log");

    lock ();
    if (logs_file.open (QIODevice::Append))
    {
        QString wstr;
        wstr = QDateTime::currentDateTime ().toString (Qt::LocaleDate);
        logs_file.write ((wstr+"! "+text+"\r\n").toUtf8 ());
        logs_file.close ();
    }
    unlock();
}
