
#include <QApplication>
#include "player_core.h"
#include "ctrl_default.h"
#include "ctrl_sync.h"
#include "setup.h"
#include "command.h"
#include "sync_udp.h"
#include "sync_tcp.h"

void default_config()
{
    cmdcfg[Command_SyncMode] = cmdcfg.ControlModeValueList[Control_All].value;
    cmdcfg[Command_SyncHost] = "";
    cmdcfg[Command_LocalType] = cmdcfg.LocalTypeValueList[Sync_Not].value;
    cmdcfg[Command_SyncThreshold] = 300;

    cmdcfg[Command_FullScreen] = false;
    cmdcfg[Command_Size] = QSize(1280, 720);

    cmdcfg[Command_Url] = "";
    cmdcfg[Command_Volume] = 1.0;
    cmdcfg[Command_Mute] = false;

    //keybord command
    cmdcfg[Command_Open] = "Key_O";
    cmdcfg[Command_TooglePause] = "Key_Space";
    cmdcfg[Command_TooglePlay] = "Key_P";
    cmdcfg[Command_ToogleMute] = "Key_M";
    cmdcfg[Command_AdjustVolume] = "Key_+ & Key_-";
    cmdcfg[Command_Seek] = "Key_Left & Key_Right";
    cmdcfg[Command_ToogleFullScreen] = "Key_Ctrl + Key_D";
    cmdcfg[Command_Quit] = "Key_Esc";

    //network command
    cmdcfg[cmdcfg.key(Command_Open, true)] = "PlayerOpen";
    cmdcfg[cmdcfg.key(Command_TooglePause, true)] = "PlayerPause";
    cmdcfg[cmdcfg.key(Command_TooglePlay, true)] = "PlayerPlay";
    cmdcfg[cmdcfg.key(Command_ToogleMute, true)] = "PlayerMute";
    cmdcfg[cmdcfg.key(Command_AdjustVolume, true)] = "PlayerVolume";
    cmdcfg[cmdcfg.key(Command_Seek, true)] = "PlayerSeekTo";
    cmdcfg[cmdcfg.key(Command_ToogleFullScreen, true)] = "PlayerFullScreen";
    cmdcfg[cmdcfg.key(Command_Quit, true)] = "PlayerQuit";
}

#define SyncConfigure "SyncPlayer.ini"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 配置命令获取
    command sync_cfg(SyncConfigure);
    QFile cfgfile(SyncConfigure);
    if (!cfgfile.exists())
        default_config();
    sync_cfg.update();

    // 安装默认界面
    ctrl_default *ctrl_ui = new ctrl_default();
    ctrl_sync *sync_ui = new ctrl_sync();
    setup *setup_ui = new setup();
    player_core core;
    //sync_udp udp_utils;
    sync_tcp tcp_utils;

    core.show();
    core.install(ctrl_ui, 2);
    core.install(sync_ui, 1);
    core.install(setup_ui, 1);

    /// 设置ui
    QObject::connect (setup_ui, &setup::sig_exit, &core, &player_core::on_exit);
    QObject::connect (setup_ui, &setup::sig_fullscreen, &core, &player_core::on_fullscreen);
    QObject::connect (setup_ui, &setup::sig_save_config, &core, &player_core::on_save_config);

    /// 默认控制ui
    QObject::connect (setup_ui, &setup::sig_exit, &tcp_utils, &sync_tcp::on_exit);
    QObject::connect (ctrl_ui, &ctrl_default::sig_open_movie, &core, &player_core::on_open_movie);
    QObject::connect (ctrl_ui, &ctrl_default::sig_command, &core, &player_core::on_command);
    QObject::connect (ctrl_ui, &ctrl_default::sig_volume, &core, &player_core::on_volume);
    QObject::connect (ctrl_ui, &ctrl_default::sig_seek, &core, &player_core::on_seekto);
    QObject::connect (&core, &player_core::sig_update_ts, ctrl_ui, &ctrl_default::on_update_ts);
    QObject::connect (&core, &player_core::sig_update_cfg, ctrl_ui, &ctrl_default::on_update_cfg);
    QObject::connect (&core, &player_core::sig_send, ctrl_ui, &ctrl_default::on_sync);
    QObject::connect (sync_ui, &ctrl_sync::sig_sync_threshold, &core, &player_core::on_sync_threshold);

    /// 同步控制ui
    QObject::connect (sync_ui, &ctrl_sync::sig_local_type, ctrl_ui, &ctrl_default::on_local_type);
    QObject::connect (&core, &player_core::sig_update_cfg, sync_ui, &ctrl_sync::on_update_cfg);
    QObject::connect (sync_ui, &ctrl_sync::sig_sync_mode, &core, &player_core::on_sync_mode);
    QObject::connect (&tcp_utils, &sync_tcp::sig_slave_list, sync_ui, &ctrl_sync::on_slave_list);
    QObject::connect (&tcp_utils, &sync_tcp::sig_slave_list, &core, &player_core::on_slave_list);
    QObject::connect (sync_ui, &ctrl_sync::sig_local_mode, &tcp_utils, &sync_tcp::on_local_mode);
    QObject::connect (&core, &player_core::sig_send, &tcp_utils, &sync_tcp::on_send);
    QObject::connect (&tcp_utils, &sync_tcp::sig_slave_recv, &core, &player_core::on_slave_recv);
    QObject::connect (&tcp_utils, &sync_tcp::sig_host_recv, &core, &player_core::on_host_recv);
    QObject::connect (sync_ui, &ctrl_sync::sig_auto_dectect, &core, &player_core::on_auto_detect);

    //QObject::connect (sync_ui, &ctrl_sync::sig_local_type, &udp_utils, &sync_udp::on_local_mode);
    //QObject::connect (&core, &player_core::sig_send, &udp_utils, &sync_udp::on_send);
    //QObject::connect (&udp_utils, &sync_udp::sig_slave_recv, &core, &player_core::on_slave_recv);
    //QObject::connect (&udp_utils, &sync_udp::sig_host_recv, &core, &player_core::on_host_recv);

    int code = a.exec();
    delete ctrl_ui;
    delete setup_ui;
    delete sync_ui;
    return code;
}

/*
 * setup ui
 *   -signal
 *    sig_exit        发出退出系统
 *    sig_fullscreen  发出切换到全屏状态
 *    sig_save_config 发出需要保存配置
 *   -slot
 *
 * default ui
 *   -signal
 *    sig_open_movie  发出打开影片
 *    sig_command     发出对播放器的播放，暂停，静音控制
 *    sig_volume      发出设置播放器的音量
 *    sig_seek        发出设置播放的位置
 *   -slot
 *    on_update_cfg   接收更新的配置
 *    on_update_ts    接收播放器更新的时间戳
 *    on_update_sync  接收本机播放器的同步数据
 *
 * sync ui
 *   -signal
 *    sig_local_type  发出设置本机的模式
 *   -slot
 *    on_update_cfg   接收更新的配置
 *    on_ping_slave   接收ping所有从机
 *    on_ping_host    接收ping主机
 *    on_slave_list   接收从机的列表
 *    on_online_host  接收主机在线状态
 *
 * sync utils
 *   -signal
 *    sig_slave_list  发出从机列表
 *    sig_ping_host   发出ping主机
 *    sig_oneline_host发出主机在线状态
 *    sig_sync_ts     发出同步时间戳
 *    sig_sync_data   发出同步数据
 *   -slot
 *    on_update_ts    接收播放器更新的时间戳
 *    on_update_sync  接收本机播放器的同步数据
 *    on_local_mode   接收设置本机的模式
 *
 * player core
 *   -signal
 *    sig_update_ts   发出播放器更新的时间戳
 *    sig_update_cfg  发出更新的配置
 *    sig_update_sync 发出本机播放器的同步数据
 *   -slot
 *    on_fullscreen   接收切换全屏状态
 *    on_exit         接收退出系统
 *    on_loop_movie   接收设置播放器的循环状态
 *    on_open_movie   接收打开影片
 *    on_toogle_play  接收播放器的播放控制
 *    on_toogle_pause 接收播放器的暂停控制
 *    on_toogle_mute  接收播放器的静音控制
 *    on_adjust_volume接收音量的调整
 *    on_volume       接收设置播放器音量
 *    on_seek         接收设置播放的位置
 *    on_seek_movie   接收播放位置的调整
 *    on_command      接收对播放器的播放，暂停，静音控制
 *    on_adjust_color 接收显示的颜色调整
 *    on_save_config  接收需要保存配置
 *
 */
