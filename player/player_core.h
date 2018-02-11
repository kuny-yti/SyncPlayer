#ifndef PLAYER_CORE_H
#define PLAYER_CORE_H

#include "command.h"
#include "sync_data.h"
#include "player.h"
#include <QObject>
#include <QUdpSocket>
#include <QThread>

#define write_logs(val) player_core::self ()->palyer ()->logs (val)
namespace impl {
class wait_play :public QThread
{
    Q_OBJECT
public:
    wait_play(qint64 = 0);
    ~wait_play();

    void wait(qint64);
    void run();
    bool is_wait()const;
private:
    qint64 ts_wait ;
    volatile bool is_wait_on;
};
class core;
}

class player_core :public QObject
{
    Q_OBJECT
public:
    player_core(QObject *p = NULL);
    ~player_core();

    static player_core *self(){return _this_;}

    player *palyer();
public:
    // 安装控制界面
    void install(QWidget *ctrl, int stretch);
    void uninstall(QWidget *ctrl);
    void show();

   void start_detect(int msec);
   void stop_detect();
signals:
    void sig_update_ts(qint64 s, double p, qint64 dur);// 主机/从机播放时更新时间戳
    void sig_update_cfg(command &cmd);                 // 更新和配置相关的界面或功能
    void sig_send(const QByteArray &data, const QHostAddress &addr, int port); // udp 发送数据

    void sig_open(const QString &url);                 // 打开影片

public slots:
    void on_update_ts(qint64 s, double p, qint64 d);   // 更新当前播放时间戳
    void on_sync_threshold(int tsms);                  // 设置同步阀值
    void on_sync_mode(eSyncModes);
    void on_auto_detect(bool);

    void on_slave_recv(const QByteArray &dat); // 从机接收的主机操作
    void on_host_recv(const QByteArray &dat);// 主机接收
    void on_slave_list(bool , const QString &addr);

    /// 退出系统
    void on_exit();
    void on_fullscreen(); // 负责全屏切换- 需要切换的要连接此信号

    /// 播放器控制
    void on_loop_movie(); //当前影片循环
    void on_open_movie(const QString &url);//打开新影片
    void on_toogle_play();//播放
    void on_toogle_pause();//暂停
    void on_toogle_mute();//静音
    void on_adjust_volume(float val);//调整音量
    void on_volume(float vol); //设置音量
    void on_seekto(double pos); //移动到相对位置
    void on_seek(qint64 pos);  //移动到绝对位置
    void on_seek_movie(float val);
    void on_command(int cmd);
    void on_adjust_color(float l, float s, float h, float e);

    void on_switch_movie(); // 界面切换影片
    void on_save_config();

private slots:
    void on_time_cfg();
    void on_switch_loop();// 主影片播放结束后切换为loop影片
    void on_dectect_play();
private:
    impl::core *impl;
    static player_core *_this_;
};

#endif // PLAYER_CORE_H
