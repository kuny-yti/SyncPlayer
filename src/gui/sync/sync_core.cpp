#include "sync_core.h"
#include "sync_tcp.h"
#include "sync_data.h"
#include <float.h>
#include <QDateTime>

#include <QTime>

namespace impl {
class core
{
public:
    sync_tcp *utils;
    QTimer cfgtimer;
    tSyncDatas data;
    command cmd;

    QString main_movie;
    QString loop_movie;

    core():
        utils(NULL),
        cfgtimer(),
        data(),
        cmd("SyncPlayer.ini")
    {

    }
};
}

sync_core::sync_core(QObject *parent):
    QObject(parent)
{
    impl = new impl::core();
    impl->utils = new sync_tcp(this);

    connect(impl->utils, &sync_tcp::sig_host_recv, this, &sync_core::on_host_recv);
    connect(impl->utils, &sync_tcp::sig_slave_recv, this, &sync_core::on_slave_recv);
    connect(&impl->cfgtimer, &QTimer::timeout, this, &sync_core::on_loadcfg);

    connect(impl->utils, &sync_tcp::sig_connect, this, &sync_core::on_disconnect);

    impl->cfgtimer.start(1000*2);
}

sync_core::~sync_core()
{
    delete impl->utils;
    delete impl;
}

QMutex sync_core::logsm;
void sync_core::logsw(const QString &str)
{
    logsm.lock();
    QFile logsf("logs.log");
    if (logsf.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        logsf.write((QDateTime::currentDateTime().toString(Qt::ISODate) + "  ").toUtf8());
        logsf.write(str.toUtf8() + "\n");
        logsf.close();
    }
    logsm.unlock();
}

static double current_length = 0; // 当前文件的总长度
static QString current_movie ;    // 当前文件名
static double current_pos = 0;    // 当前播放的位置
static bool is_playend = false;   // 是否播放结束
static qint64 last_upts = 0;      // 上一次更新播放位置的时间
void sync_core::on_update_length(double v)
{
    if (abs(v) > DBL_EPSILON)
    {
        current_length = v;
        is_playend = false;
        coreLogsWrite(QString("update length: length=%1").arg(v));
    }
    last_upts = QDateTime::currentMSecsSinceEpoch();
}
void sync_core::on_update_pos(double v)
{
    static bool has_host = cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value;

    if (abs(current_length) < DBL_EPSILON)//若长度不大于零或不为主机模式则返回
        return ;

    if (abs(current_length - v) <= DBL_EPSILON)//指定播放结束
        is_playend = true;
    else
        is_playend = false;

    current_pos = v;
    last_upts = QDateTime::currentMSecsSinceEpoch();

    if (!has_host)
        return ;

    impl->data.request_ts = QDateTime::currentMSecsSinceEpoch();
    impl->data.head = Sync_Request;

    if (is_playend) // 若播放结束
    {
        if (impl->main_movie == current_movie) // 若为主影片则切换到循环影片
        {
            impl->data.cmd = Sync_SwitchMovie;
            impl->data.url = impl->loop_movie;
            emit sig_select(impl->loop_movie);
            current_movie = impl->loop_movie;
        }
        else if (impl->loop_movie == current_movie) // 若为循环影片则从零播放
        {
            impl->data.cmd = Sync_Seek;
            impl->data.url = impl->loop_movie;
            impl->data.seek = 0.0;
            emit sig_seek(0);
            current_movie = impl->loop_movie;
            //emit sig_select(impl->loop_movie);
        }
    }
    else // 发送时间戳
    {
        impl->data.url = current_movie;
        impl->data.cmd = Sync_Timestamp;
        impl->data.timestamp = (int)v;
    }

    impl->utils->on_send(impl->data.data(), QHostAddress(), 998);
}
void sync_core::on_switch_movie()
{
    static bool has_host = cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value;

    if (current_movie == impl->main_movie || !has_host)
        return ;

    if (current_movie == impl->loop_movie)
    {
        impl->data.request_ts = QDateTime::currentMSecsSinceEpoch();
        impl->data.head = Sync_Request;
        impl->data.cmd = Sync_SwitchMovie;
        impl->data.url = impl->main_movie;
        emit sig_select(impl->main_movie);
        current_movie = impl->main_movie;
        impl->utils->on_send(impl->data.data(), QHostAddress(), 998);
    }
}
void sync_core::on_host_recv(const QByteArray &ba)
{

}
void sync_core::on_disconnect(bool s, const QString &ip)
{
    if (!s)
        current_movie = QString();
    coreLogsWrite(QString("network (state=%1): ip=%2").arg(s).arg(ip));
}
void sync_core::on_slave_recv(const QByteArray &ba)
{
    QByteArray sdat = ba;
    tSyncDatas sd(sdat);
    if (sd.head != Sync_Request)
        return ;

    impl->data.mode = sd.mode;

    if (impl->data.mode == Sync_ModeAll || impl->data.mode == Sync_ModeContorl)
    {
        switch ((int)sd.cmd)
        {
        case Sync_SwitchMovie:
            impl->data.url = sd.url;
            current_movie = sd.url;
            emit sig_select(sd.url);
            coreLogsWrite(QString("slave recv(switch movie): url=%1").arg(sd.url));
            break;

        case Sync_Seek:
            if (sd.url == current_movie)
            {
                if (is_playend)
                {
                    emit sig_select(sd.url);
                    emit sig_seek(sd.seek);
                    impl->data.url = sd.url;
                    current_movie = sd.url;
                    coreLogsWrite(QString("slave recv(seek[playend=true]): url=%1").arg(sd.url));
                }
                else
                {
                    emit sig_seek((int)sd.seek);
                    coreLogsWrite(QString("slave recv(seek[playend=false]): url=%1").arg(sd.url));
                }
            }
            else
            {
                emit sig_select(sd.url);
                emit sig_seek(sd.seek);
                impl->data.url = sd.url;
                current_movie = sd.url;
                coreLogsWrite(QString("slave recv(seek[url != movie]): url=%1").arg(sd.url));
            }
            break;
        case Sync_Timestamp:
            // 若主机播放影片和本机不同时打开新的影片
            if (sd.url != current_movie)
            {
                emit sig_select(sd.url);
                impl->data.url = sd.url;
                current_movie = sd.url;
                emit sig_seek(sd.timestamp);
                coreLogsWrite(QString("slave recv(timestamp[url != movie]): url=%1").arg(sd.url));
            }
            else if (is_playend || (QDateTime::currentMSecsSinceEpoch() - last_upts > 5*1000))
            {
                emit sig_select(sd.url);
                impl->data.url = sd.url;
                current_movie = sd.url;
                emit sig_seek(sd.timestamp);
                coreLogsWrite(QString("slave recv(timestamp[is_playend=%2 , last_upts > 5s]): url=%1 and seek=%3").arg(sd.url).arg(is_playend).arg(sd.timestamp));
                last_upts = QDateTime::currentMSecsSinceEpoch();
            }
            break;
        default:
            break;
        }
    }
    // 进行时间戳矫正
    if (impl->data.mode == Sync_ModeAll || impl->data.mode == Sync_ModeTimestamp)
    {
        /*
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
            last_wait =false;*/

    }
}

void sync_core::on_loadcfg()
{
    impl->cfgtimer.stop();

    impl->data.threshold = cmdcfg[Command_SyncThreshold].toInt ();

    // 启动是否全屏
    emit sig_view();
    if (cmdcfg[Command_FullScreen].toBool())
        emit sig_full();

    // 取出配置的main影片和loop影片
    QString main_movie = cmdcfg[Command_Url].toString();
    QStringList all_movie = cmdcfg.keys(Command_Url);
    for (int i = 0; i < all_movie.count(); ++i)
    {
        if (all_movie[i] != main_movie)
            impl->loop_movie = all_movie[i];
    }
    impl->loop_movie = cmdcfg[impl->loop_movie].toString();
    impl->main_movie = main_movie;

    // 是否为主机，若为主机则启动切换和播放main影片
    if (cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value)
    {
        impl->utils->on_local_mode(Sync_Host, QHostAddress());
        emit sig_select(impl->loop_movie);
        emit sig_select(impl->main_movie);

        impl->data.request_ts = QDateTime::currentMSecsSinceEpoch();
        impl->data.cmd = Sync_SwitchMovie;
        impl->data.head = Sync_Request;
        impl->data.url = impl->main_movie;
        current_movie = impl->main_movie;

        impl->utils->on_send(impl->data.data(), QHostAddress(), 998);
    }
    else if (cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Slave].value)
    {
        QString host_ip = cmdcfg[Command_SyncHost].toString();
        impl->utils->on_local_mode(Sync_Slave, QHostAddress(host_ip));
    }

    disconnect(&impl->cfgtimer, &QTimer::timeout, this, &sync_core::on_loadcfg);
    connect(&impl->cfgtimer, &QTimer::timeout, this, &sync_core::on_detect);
    impl->cfgtimer.start(2000);
}

void sync_core::on_detect()
{
    static bool has_host = cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value;

    if (has_host)
    {
        if (QDateTime::currentMSecsSinceEpoch() - last_upts > (1000*10) )
        {
            impl->data.request_ts = QDateTime::currentMSecsSinceEpoch();
            impl->data.head = Sync_Request;
            if (!current_movie.isEmpty())
                impl->data.url = current_movie;

            if (impl->data.url.isEmpty())
                impl->data.url = impl->main_movie;
            impl->data.cmd = Sync_SwitchMovie;
            emit sig_select(impl->data.url);
            current_movie = impl->data.url;
            impl->utils->on_send(impl->data.data(), QHostAddress(), 998);
        }
    }
    else
    {
        if (QDateTime::currentMSecsSinceEpoch() - last_upts > (1000*10) )
        {
            is_playend = true;
        coreLogsWrite(QString("detect update timestamp (playend=%1): last_upts=%2, current_ts=%3").arg(is_playend).arg(last_upts).arg(QDateTime::currentMSecsSinceEpoch()));
        }
    }
}
