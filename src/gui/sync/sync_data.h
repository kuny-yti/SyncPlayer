#ifndef SYNC_DATA
#define SYNC_DATA

#include "QString"
#include <QVector4D>
#include <QDataStream>
#include <QByteArray>

typedef enum
{
    Display_sRGB,
    Display_AdobeRGB
}eDisplayPixelFormats;

typedef enum
{
    Sync_ModeContorl = 1,
    Sync_ModeTimestamp = 2,
    Sync_ModeAll = Sync_ModeContorl | Sync_ModeTimestamp
}eSyncModes;

typedef enum
{
    State_Play = 0x01,
    State_Pause = 0x02,
    State_Mute = 0x04,
    State_FullScreen = 0x08
}eMovieStates;

typedef enum
{
    Sync_Request = 0x200,// 主机下发请求
    Sync_Answer,         // 从机发回应答
    Sync_Quit,           // 推出请求

    Sync_Threshold=0x100,// 设置同步阀值(int64)
    Sync_Mode,
    Sync_Timestamp,      // 发布同步时间戳(int64)
    Sync_SwitchMovie,    // 切换影片(char*)
    Sync_PlayState,      // 播放状态(bool)
    Sync_PauseState,     // 暂停状态(bool)
    Sync_MuteState,      // 静音状态(bool)
    Sync_FullScreenState,// 全屏状态(bool)
    Sync_AdjustColor,    // 调整颜色(vec4)
    Sync_AdjustVolum,    // 调整音量(float)
    Sync_SeekTo,         // 移动到指定位置(float)
    Sync_Seek,
    Sync_DisplayFormat,  // 设置显示的颜色格式 (sRGB或AdobeRGB)
    Sync_DisplayQuality  // 设置显示的图像质量(int)
}eSyncCommands;

typedef struct _tSyncDatas_
{
    eSyncCommands head; //同步头
    eSyncCommands cmd;  //同步命令
    eSyncModes mode;
    QString url;        //当前播放的影片
    QString addrs;      //当前从机的地址
    qint64 threshold;  //同步阀值，误差超过此值则进行矫正
    qint64 timestamp;  //播放器当前时间戳
    qint64 frame_count;
    float frame_rate;
    int state;          //播放器的状态 全屏|静音|暂停|播放
    QVector4D color;    //播放器显示调整颜色
    float volume;       //播放器当前音量
    double seek;        //播放器跳到相对位置
    int format;         //播放器显示颜色格式
    int quality;        //播放器显示图像质量

    qint64 system_ts;   //发送者标准时间戳
    qint64 request_ts;  //请求时间戳
    qint64 answer_ts;   //应答时间戳

    _tSyncDatas_():
        head(Sync_Request),
        cmd(Sync_Request),
        mode(Sync_ModeContorl),
        url(),
        addrs(),
        threshold(60),//msec
        timestamp(0),
        frame_count(0),
        frame_rate(0.f),
        state(0),
        color(),
        volume(1.0),
        seek(0.0),
        format(Display_sRGB),
        quality(4),
        system_ts(0),
        request_ts(0),
        answer_ts(0)
    {}
    _tSyncDatas_(QByteArray &in):
        head(Sync_Request),
        cmd(Sync_Request),
        mode(Sync_ModeContorl),
        url(),
        addrs(),
        threshold(60),
        timestamp(0),
        frame_count(0),
        frame_rate(0.f),
        state(0),
        color(),
        volume(1.0),
        seek(0.0),
        format(Display_sRGB),
        quality(4),
        system_ts(0),
        request_ts(0),
        answer_ts(0)
    {
        int tmp = 0;
        QDataStream read(&in, QIODevice::ReadOnly);
        read >> tmp; head = (eSyncCommands)tmp;
        read >> tmp; cmd = (eSyncCommands)tmp;
        read >> tmp; mode = (eSyncModes)tmp;
        read >> url;
        read >> addrs;
        read >> threshold;
        read >> timestamp;
        read >> frame_count;
        read >> frame_rate;
        read >> state;
        read >> color;
        read >> volume;
        read >> seek;
        read >> format;
        read >> quality;
        read >> system_ts;
        read >> request_ts;
        read >> answer_ts;
    }

    QByteArray data()const
    {
        QByteArray out;
        QDataStream write(&out, QIODevice::WriteOnly);
        write << head;
        write << cmd;
        write << mode;
        write << url;
        write << addrs;
        write << threshold;
        write << timestamp;
        write << frame_count;
        write << frame_rate;
        write << state;
        write << color;
        write << volume;
        write << seek;
        write << format;
        write << quality;
        write << system_ts;
        write << request_ts;
        write << answer_ts;
        return out;
    }
}tSyncDatas;

#endif // SYNC_DATA

