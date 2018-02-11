#include "player_core.h"

#include "player.h"
#include "player_ui.h"
#include "sync_data.h"

#include <QApplication>
#include <QDebug>
#include <QDateTime>
#include <QTimer>

namespace impl {

wait_play::wait_play(qint64 wts):
    QThread(),ts_wait(wts)
{
}
wait_play::~wait_play()
{
}
void wait_play::wait(qint64 wts)
{
    if (this->isRunning ())
        return;

    ts_wait = wts;
    this->start ();
}
bool wait_play::is_wait()const
{
    return is_wait_on;
}
void wait_play::run()
{
    is_wait_on = true;
    player_core::self ()->palyer ()->pause ();
    qint64 tempts = get_microseconds ();
    while ((get_microseconds () - tempts) < ts_wait) {}
    player_core::self ()->palyer ()->pause (false);
    is_wait_on = false;
}

class core
{
public:
    player_ui *play_ui;
    player *play_core;
    video_output *vo_opengl;
    audio_output *ao_openal;
    tSyncDatas sync_data;

    QTimer  cfg_time;
    QTimer  switch_time;
    QTimer  dectect_time;
    QString main_url;
    QString loop_url;

    QMutex  ope_mutex;

    bool is_auto_detect;

    core():
        play_ui(NULL),
        play_core(NULL),
        vo_opengl(NULL),
        ao_openal(NULL),
        sync_data(),
        is_auto_detect(true)
    {
        sync_data.system_ts = get_microseconds();
    }
};
}

static qint64 last_upts = 0;
static qint64 last_host_rets = 0;
static quint64 slast_ts = get_microseconds ();
void static_update_ts(qint64 s, double p)
{
    emit player_core::self()->sig_update_ts(s, p, player_core::self()->palyer ()->duration ());

    last_upts = get_microseconds ();
}

player_core *player_core::_this_ = NULL;

player_core::player_core(QObject *p):
    QObject(p)
{
    impl = new impl::core();
    _this_ = this;

    impl->ao_openal = new audio_output();
    impl->vo_opengl = new video_output();
    impl->play_core = new player(impl->vo_opengl, impl->ao_openal);
    impl->play_core->set(static_update_ts);

    impl->play_ui = new player_ui();
    impl->play_ui->install(impl->vo_opengl, 4);
    impl->play_ui->resize(1280, 720);

    connect(impl->play_ui, &player_ui::sig_open_movie, this, &player_core::on_open_movie);
    connect(impl->play_ui, &player_ui::sig_adjust_volume, this, &player_core::on_adjust_volume);
    connect(impl->play_ui, &player_ui::sig_command, this, &player_core::on_command);
    connect(impl->play_ui, &player_ui::sig_seek_movie, this, &player_core::on_seek_movie);
    connect(impl->play_ui, &player_ui::sig_switch_movie, this, &player_core::on_switch_movie);//主机点击切换影片
    connect(impl->vo_opengl, &video_output::sig_key_event, impl->play_ui, &player_ui::on_key_event);
    connect(this, &player_core::sig_open, this, &player_core::on_open_movie);
    connect(this, &player_core::sig_update_ts, this, &player_core::on_update_ts);

    connect(&impl->cfg_time, &QTimer::timeout, this, &player_core::on_time_cfg);
    connect(&impl->switch_time, &QTimer::timeout, this, &player_core::on_switch_loop);
    connect (&impl->dectect_time, &QTimer::timeout, this, &player_core::on_dectect_play);
    impl->cfg_time.start(2000);
}

player_core::~player_core()
{
    try {
    delete impl->ao_openal;
    delete impl->vo_opengl;
    delete impl->play_core;
    delete impl->play_ui;
    delete impl;
    }
    catch (const QString &str)
    {
        qDebug() << str;
    }
}

player *player_core::palyer()
{
    return impl->play_core;
}
void player_core::install(QWidget *ctrl, int stretch)
{
    impl->play_ui->install(ctrl, stretch);
}
void player_core::uninstall(QWidget *ctrl)
{
    impl->play_ui->uninstall(ctrl);
}
void player_core::show()
{
    impl->play_ui->show();
}

void player_core::on_save_config()
{
    write_logs ("save config.");
    cmdcfg.update();
}
void player_core::on_fullscreen()
{
    impl->play_ui->on_fullscreen();
    impl->sync_data.cmd = Sync_FullScreenState;
    if (impl->play_ui->is_fullscreen())
        impl->sync_data.state = State_FullScreen;
    else
        impl->sync_data.state = 0;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);

    write_logs ("full screen" + QString::number (impl->play_ui->is_fullscreen ()));
}
void player_core::on_exit()
{
    if (impl->play_core->is_play())
        impl->play_core->play(false);
    impl->sync_data.cmd = Sync_Quit;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    QThread::sleep (2);
    write_logs ("exit system");
    qApp->exit();
}

void player_core::start_detect(int msec)
{
    /*if (!impl->is_auto_detect)
        return ;
    if (impl->dectect_time.isActive ())
        impl->dectect_time.stop ();
    last_host_rets = get_microseconds ();
    last_upts = get_microseconds ();
    impl->dectect_time.start (msec);*/
}
void player_core::stop_detect()
{
    /*if (!impl->is_auto_detect)
        return ;
    if (impl->dectect_time.isActive ())
        impl->dectect_time.stop ();

    last_host_rets = get_microseconds ();
    last_upts = get_microseconds ();*/
}
// 检测若超过10s没有播放则会转为主影片播放
void player_core::on_dectect_play()
{
    static bool not_host = cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Slave].value;
    if (!impl->is_auto_detect)
        return ;
   if (get_microseconds () - last_upts > 1000*1000*3)//10s
   {
       write_logs ("dectect 10s not play the open:"+impl->play_core->curren_file ());
       if (impl->play_core->curren_file () == impl->loop_url)
       {
           on_open_movie(impl->main_url);
           on_toogle_play();
       }
       else
       {
           on_open_movie(impl->loop_url);
           on_toogle_play();
       }
   }
   else if ((get_microseconds () - last_host_rets > 1000*1000*3) && not_host &&
            (!impl->play_core->is_play ()|| impl->play_core->is_pause ()))
   {
       write_logs ("dectect 10s not recv host cmd. ope:"+impl->play_core->curren_file ());
       if (impl->play_core->curren_file () == impl->loop_url)
       {
           on_open_movie(impl->main_url);
           on_toogle_play();
       }
       else
       {
           on_open_movie(impl->loop_url);
           on_toogle_play();
       }
   }
}
void player_core::on_update_ts(qint64 s, double p, qint64 d)
{
    static bool is_upts = cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value;

    if (!is_upts)
        return ;
    impl->sync_data.request_ts = get_microseconds ();
    impl->sync_data.cmd = Sync_Timestamp;
    impl->sync_data.head = Sync_Request;
    impl->sync_data.seek =  p;
    impl->sync_data.timestamp = impl->play_core->tell ();
    impl->sync_data.url = impl->play_core->curren_file ();
    impl->sync_data.state = State_Play;
    impl->sync_data.frame_count = impl->play_core->frame ();
    impl->sync_data.frame_rate = impl->play_core->frame_rate ();

    if (s == -2)
    {
        if (impl->loop_url == impl->play_core->curren_file ())
        {
            write_logs ("update ts: -2");
            on_toogle_play ();
            on_seekto (0.0);
        }
        else if (impl->main_url == impl->play_core->curren_file ())
        {
            write_logs ("update ts: -2-open");
            emit sig_open (impl->loop_url);
            on_toogle_play ();
            on_seekto (0.0);
        }
    }
    else if (s == 0 && impl->loop_url == impl->play_core->curren_file ())
    {
        write_logs ("update ts: 0");
        on_seekto (0.0);
    }
    else
    {
        //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
        emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    }

    /*
    if ((d - s) < 1000*1000 && !impl->switch_time.isActive ())
    {
        write_logs("switch movie start:(ms) "+QString::number ((d - s) / 1000));
        impl->switch_time.start ((d - s) / 1000);
    }*/
}
void player_core::on_slave_recv(const QByteArray &dat)
{
    static impl::wait_play wait_host;
    static qint64 detect_ts = 0;
    static bool last_wait = false;

    QByteArray sync_rv = dat;
    tSyncDatas sd(sync_rv);
    if (sd.head != Sync_Request)
        return ;
    impl->sync_data.mode = sd.mode;

    if (impl->sync_data.mode == Sync_ModeAll || impl->sync_data.mode == Sync_ModeContorl)
    {
        switch ((int)sd.cmd)
        {
        case Sync_FullScreenState:
            write_logs ("recv full screen state" + QString::number (sd.state & State_FullScreen));
            if (!impl->play_ui->is_fullscreen() && (sd.state & State_FullScreen))
                on_fullscreen();
            else if (impl->play_ui->is_fullscreen() && !(sd.state & State_FullScreen))
                on_fullscreen();
            break;
        case Sync_PlayState:
            // 若主机播放影片和当前影片相同则控制播放状态
            if (sd.url == impl->sync_data.url)
            {
                write_logs ("recv play state" +QString::number (~(sd.state & State_Play)));
                impl->play_core->play(~(sd.state & State_Play));
                if ((sd.state & State_Play))
                    start_detect (2*1000);
                else
                    stop_detect ();
            }
            // 若主机和本机播放影片不同则打开新的影片并设置播放状态
            else if (sd.url != impl->sync_data.url)
            {
                write_logs ("recv play state [url !=]" + QString::number (~(sd.state & State_Play)));
                emit sig_open(sd.url);
                impl->play_core->play(~(sd.state & State_Play));
                if ((sd.state & State_Play))
                    start_detect (2*1000);
                else
                    stop_detect ();
            }
            break;
        case Sync_PauseState:
            write_logs ("recv pause state" + QString::number (~(sd.state & State_Pause)));
            impl->play_core->pause(~(sd.state & State_Pause));
            if ((sd.state & State_Pause))
                start_detect (2*1000);
            else
                stop_detect ();
            break;
        case Sync_MuteState:
            write_logs ("recv mute state" + QString::number (~(sd.state & State_Pause)));
            impl->play_core->mute(~(sd.state & State_Mute));
            break;
        case Sync_SeekTo:
            write_logs ("recv seekto" + QString::number (sd.seek));
            on_seekto(sd.seek);
            break;
        case Sync_Seek:
            write_logs ("recv seek" + QString::number (sd.timestamp));
            on_seek(sd.timestamp);
            break;
        case Sync_Quit:
            write_logs ("recv quit");
            on_exit();
            break;
        case Sync_AdjustVolum:
            write_logs("recv adjust volum" + QString::number (sd.volume));
            on_volume(sd.volume);
            break;
        case Sync_SwitchMovie:
            write_logs ("recv switch movie " + sd.url);
            emit sig_open(sd.url);
            break;
        case Sync_Mode:
            write_logs ("recv set sync mode:"+QString::number (sd.mode));
            impl->sync_data.mode = sd.mode;
            break;
        case Sync_Timestamp:
            // 若主机播放影片和本机不同时打开新的影片
            if (sd.url != impl->sync_data.url)
            {
                write_logs ("recv timestamp [url !=] open:"+impl->sync_data.url);
                emit sig_open(sd.url);
            }
            // 若主机是播放状态则本机未播放时则设置播放
            else if (!impl->play_core->is_play())
            {
                write_logs ("recv timestamp not play:"+QString::number (impl->play_core->is_play())+" seek:"+QString::number (sd.timestamp));
                impl->play_core->play();
                impl->play_core->seek (sd.timestamp);
                start_detect (2*1000);
            }
            break;
        default:
            break;
        }
    }
    // 进行时间戳矫正
    if (impl->sync_data.mode == Sync_ModeAll || impl->sync_data.mode == Sync_ModeTimestamp)
    {
        if ((get_microseconds () - detect_ts) > (1000*1000*10))
        {
            if (sd.cmd == Sync_Timestamp && !wait_host.is_wait () && !last_wait)
            {
                qint64 local_ts = impl->play_core->tell ();
                qint64 ts_wait = local_ts - sd.timestamp;
                if((ts_wait > (impl->sync_data.threshold * 1000 * 1000)) || //若阀值高于100倍(s级别)时使用seek
                        (ts_wait < 0 && (qAbs(ts_wait) > (impl->sync_data.threshold * 1000))))//比主机慢则seek
                {
                    write_logs ("seek "+ QString::number (ts_wait));
                    on_seek (sd.timestamp);
                }
                else if (ts_wait > 0 && (ts_wait > impl->sync_data.threshold * 1000)) //比主机快则wait
                {
                    write_logs ("wait "+ QString::number (ts_wait) +
                                           "\r\n    host ts:"+ QString::number (sd.timestamp)+
                                           " local ts:"+ QString::number (impl->play_core->tell ()));
                    wait_host.wait (ts_wait);
                }
                else
                {
                    detect_ts = get_microseconds ();
                    last_wait =true;
                }
            }
            else if (last_wait)
                last_wait =false;
        }
    }

    last_host_rets = get_microseconds ();
}

void player_core::on_auto_detect (bool s)
{
    /*impl->is_auto_detect = s;
    if (s && !impl->dectect_time.isActive () && impl->play_core->is_play () && !impl->play_core->is_pause ())
        impl->dectect_time.start (1000*5);
    else if (s && impl->dectect_time.isActive ())
        impl->dectect_time.stop ();*/
}
void player_core::on_slave_list(bool s, const QString &addr)
{
    if (s)
        write_logs("slave connected: "+addr);
    else
        write_logs("slave disconnected: "+addr);
}
/// 接受的从机同步数据
void  player_core::on_host_recv(const QByteArray &dat)
{
    //QByteArray ba = dat;
    //tSyncDatas sd(ba);
}
// 主机会操作
void player_core::on_sync_mode(eSyncModes sm)
{
    impl->sync_data.cmd = Sync_Mode;
    impl->sync_data.mode = sm;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    write_logs ("ope sync mode:"+QString::number (sm));
}
void player_core::on_sync_threshold(int tsms)
{
    impl->sync_data.cmd = Sync_Threshold;
    impl->sync_data.threshold = tsms;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
   // emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    write_logs ("ope sync threshold:"+QString::number (tsms));
}
void player_core::on_command(int cmd)
{
    try {
    if (cmd == Command_TooglePause)
        on_toogle_pause();
    else if (cmd == Command_ToogleMute)
        on_toogle_mute();
    else if (cmd == Command_TooglePlay)
        on_toogle_play();
    else if (cmd == Command_Quit)
        on_exit();
    }
    catch (const QString &str)
    {
        qDebug() <<"cmd:" << cmd << str;
    }
    write_logs ("ope command:"+QString::number (cmd));
}
void player_core::on_loop_movie()
{
    write_logs ("ope loop current movie");
    impl->play_core->loop(LOOP_CURRENT_FILE);
}
void player_core::on_open_movie(const QString &url)
{
    if (url.isEmpty())
        return ;

    if (!impl->ope_mutex.tryLock(60))
        return ;
    try {
        impl->play_core->open(url);

        if ((impl->loop_url == url) &&
                (cmdcfg[Command_LocalType].toString() ==
                 cmdcfg.LocalTypeValueList[Sync_Host].value))
            impl->play_core->loop(LOOP_CURRENT_FILE);
        else
        {
            impl->play_core->loop(LOOP_NO);
        }
    }
    catch (const QString &str)
    {
        impl->ope_mutex.unlock();
        qDebug() <<"open" << url << str;
    }

    impl->sync_data = tSyncDatas();
    impl->sync_data.cmd = Sync_SwitchMovie;
    impl->sync_data.url = url;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    impl->ope_mutex.unlock();
    write_logs ("ope open movie:"+url);
}
void player_core::on_toogle_play()
{

    if (!impl->play_core->is_open () && !impl->play_core->curren_file ().isEmpty ())
        on_open_movie (impl->play_core->curren_file ());

    if (!impl->ope_mutex.tryLock(60))
        return ;
    try {
        if (!impl->play_core->is_play())
        {
            start_detect (1000*5);
            impl->play_core->play ();
            write_logs ("ope play true");
        }
        else
        {
            stop_detect ();
            impl->play_core->play (false);
            write_logs ("ope play false");
        }
    }
    catch (const QString &str)
    {
        impl->ope_mutex.unlock();
        qDebug() <<"play"  << str;
    }

    impl->sync_data.cmd = Sync_PlayState;
    if (impl->play_core->is_play())
        impl->sync_data.state = State_Play;
    else
        impl->sync_data.state = 0;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();

    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);

    impl->ope_mutex.unlock();
}
void player_core::on_toogle_pause()
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    try {
        if (impl->play_core->is_pause ())
        {
            start_detect (1000*5);
            impl->play_core->pause (false);
            write_logs ("ope pause false");
        }
        else
        {
            stop_detect ();
            impl->play_core->pause (true);
             write_logs ("ope pause true");
        }
    }
    catch (const QString &str)
    {
        qDebug() <<"pause" << str;
    }

    impl->sync_data.cmd = Sync_PauseState;
    if (impl->play_core->is_pause())
        impl->sync_data.state = State_Pause;
    else
        impl->sync_data.state = 0;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    impl->ope_mutex.unlock();

}
void player_core::on_toogle_mute()
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    try {
    impl->play_core->toggle_mute();
    }
    catch (const QString &str)
    {
        impl->ope_mutex.unlock();
         qDebug() <<"mute" << str;
    }

    impl->sync_data.cmd = Sync_MuteState;
    if (impl->play_core->is_mute())
        impl->sync_data.state = State_Mute;
    else
        impl->sync_data.state = 0;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    impl->ope_mutex.unlock();
    write_logs ("ope toogle mute");
}
void player_core::on_adjust_volume(float val)
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    float curr_volume = impl->play_core->volume();
    curr_volume += val;
    if (curr_volume > 1.0)
        curr_volume = 1.0;
    else if (curr_volume < 0.0)
        curr_volume = 0.0;
    impl->play_core->volume(curr_volume);

    impl->sync_data.cmd = Sync_AdjustVolum;
    impl->sync_data.volume = curr_volume;
    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    impl->ope_mutex.unlock();

    write_logs ("ope adjust volume:"+QString::number (curr_volume));
}
void player_core::on_volume(float val)
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    impl->play_core->volume(val);
    impl->sync_data.cmd = Sync_AdjustVolum;
    impl->sync_data.volume = val;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    write_logs ("ope volume:"+QString::number (val));
    impl->ope_mutex.unlock();
}
void player_core::on_seekto(double pos)
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    impl->sync_data.cmd = Sync_SeekTo;
    impl->sync_data.seek = pos;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    impl->play_core->seek(pos);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);

    write_logs ("ope seekto:"+QString::number (pos));
    impl->ope_mutex.unlock();
}
void player_core::on_seek(qint64 pos)
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    impl->play_core->seek(pos);
    impl->sync_data.cmd = Sync_Seek;
    impl->sync_data.timestamp = pos;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    write_logs ("ope seek:"+QString::number (pos));
    impl->ope_mutex.unlock();
}
void player_core::on_seek_movie(float val)
{
    if (!impl->ope_mutex.tryLock(60))
        return ;
    float curr_volume = impl->play_core->pos();
    curr_volume += val;
    if (curr_volume > 1.0)
        curr_volume = 1.0;
    else if (curr_volume < 0.0)
        curr_volume = 0.0;
    impl->play_core->seek(curr_volume);

    impl->sync_data.cmd = Sync_SeekTo;
    impl->sync_data.seek = curr_volume;

    impl->sync_data.head = Sync_Request;
    impl->sync_data.request_ts = get_microseconds ();
    emit sig_send(impl->sync_data.data (), QHostAddress(), 999);
    //emit sig_send(impl->sync_data.data (), QHostAddress::Broadcast, 999);
    write_logs ("ope seek movie:"+QString::number (curr_volume));
    impl->ope_mutex.unlock();
}
void player_core::on_adjust_color(float l, float s, float h, float e)
{

}
void player_core::on_switch_movie()
{
    if (impl->main_url != impl->sync_data.url &&!impl->main_url.isEmpty())
    {
        write_logs("switch_movie(main):"+impl->main_url);
        emit sig_open (impl->main_url);
        on_toogle_play();
        on_seekto (0.0);
        slast_ts = get_microseconds ();
    }
}

// 切换影片
void player_core::on_switch_loop()
{
    impl->switch_time.stop();
    write_logs("switch_loop:"+impl->loop_url+" "+impl->sync_data.url);

    if (impl->loop_url.isEmpty())
        return ;

    // 当播放的为main影片则切换为loop影片
    if (impl->loop_url != impl->sync_data.url)
    {
        write_logs("switch_movie(loop):"+impl->loop_url);
        sig_open (impl->loop_url);
        on_toogle_play();
        on_seekto (0.0);
    }
}

void player_core::on_time_cfg()
{
    impl->cfg_time.stop();

    impl->sync_data.threshold = cmdcfg[Command_SyncThreshold].toInt ();
    impl->play_ui->resize(cmdcfg[Command_Size].toSize());

    // 启动是否全屏
    if (cmdcfg[Command_FullScreen].toBool())
    {
        on_fullscreen();
    }

    // 取出配置的main影片和loop影片
    QString main_movie = cmdcfg[Command_Url].toString();
    QStringList all_movie = cmdcfg.keys(Command_Url);
    for (int i = 0; i < all_movie.count(); ++i)
    {
        if (all_movie[i] != main_movie)
            impl->loop_url = all_movie[i];
    }
    impl->loop_url = cmdcfg[impl->loop_url].toString();
    impl->main_url = main_movie;

    // 是否为主机，若为主机则启动切换和播放main影片
    if (cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value)
    {
        on_open_movie(impl->main_url);
        on_toogle_play();
    }
    //impl->is_auto_detect = cmdcfg[Command_SyncAutoDetect].toBool ();
    emit sig_update_cfg(cmdcfg);
}
