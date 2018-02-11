#ifndef SYNC_CONFIG_H
#define SYNC_CONFIG_H

#include <QSettings>
#include <QKeyEvent>

const QString video_stl =
        QString("Media Files (*.ASF *.AVI *.FLV *.MOV *.MP4 *.3GP *.RMVB *.WMV *.MKV *.MPG *.VOB *.SWF");

#include "type.h"

typedef enum
{
    Control_All, //控制从机(播放状态、播放同步、选择媒体、显示状态、音量、色彩调整)
    Control_Timestamp,//控制从机(播放同步)
    Control_Control,//控制从机(播放状态、媒体选择)
    Control_State,//控制从机(播放状态、音量)
    Control_Advanced,//控制从机(显示状态、音量、色彩调整)

    Control_Count
}eControlModes;

typedef enum
{
    Sync_Not,   //脱机模式
    Sync_Host,  //主机模式
    Sync_Slave, //从机模式

    Sync_Count
}eLocalTypes;

typedef enum
{
    Command_Url,        // 当前要播放的媒体url
    Command_Volume,      // 当前播放器的音量
    Command_Mute,       // 当前播放器静音

    Command_Open,       // 播放器打开文件
    Command_TooglePause,// 播放器的暂停/恢复
    Command_TooglePlay, // 播放器的播放/停止
    Command_ToogleMute, // 播放器的静音/恢复
    Command_AdjustVolume,// 播放器的当前音量
    Command_Seek,       // 播放器移动位置
    Command_ToogleFullScreen,
    Command_Quit,

    Command_FullScreen, // 全屏状态
    Command_Size,       // 界面的尺寸

    Command_SyncHost,   // 同步的主机地址 (IPv4:127.0.0.1)
    Command_LocalHost,
    Command_SyncMode,   // 同步的控制模式 (eControlModes)
    Command_SyncAutoDetect, // 同步是否开启自动检测播放
    Command_LocalType,  // 当前主机工作的模式 (eLocalTypes)
    Command_SyncThreshold,

    Command_Count
}eCommandLists;

typedef enum
{
    Key_NotMode,
    Key_SingleMode,
    Key_OrMode, //"|"
    Key_AndMode,//"&"
    Key_ModifierMode//"+"
}eKeyModes;
class command_key
{
public:
    const char *const KeyBoardKey = "Key";
    const char *const MouseKey = "Mouse";

    const char *const KeySplit = "_";
    const char *const KeyOrSplit = "|";
    const char *const KeyAndSplit = "&";
    const char *const KeyModifierSplit = "+";

    const struct
    {
        const char* const keyval;
        const int keycode;
    } KeyBoardCodeList[29] =
    {
    {"LEFT",     Qt::Key_Left},
    {"RIGHT",    Qt::Key_Right},
    {"UP",       Qt::Key_Up},
    {"DOWN",     Qt::Key_Down},
    {"CTRL",     Qt::ControlModifier},
    {"ALT",      Qt::AltModifier},
    {"SHIFT",    Qt::ShiftModifier},
    {"HOME",     Qt::Key_Home},
    {"END",      Qt::Key_End},
    {"PGUP",     Qt::Key_PageUp},
    {"PAGEUP",   Qt::Key_PageUp},
    {"PGDO",     Qt::Key_PageDown},
    {"PGDOWN",   Qt::Key_PageDown},
    {"PAGEDOWN", Qt::Key_PageDown},
    {"ESC",      Qt::Key_Escape},
    {"ESCAPE",   Qt::Key_Escape},
    {"SPACE",    Qt::Key_Space},
    {"F1",       Qt::Key_F1},
    {"F2",       Qt::Key_F2},
    {"F3",       Qt::Key_F3},
    {"F4",       Qt::Key_F4},
    {"F5",       Qt::Key_F5},
    {"F6",       Qt::Key_F6},
    {"F7",       Qt::Key_F7},
    {"F8",       Qt::Key_F8},
    {"F9",       Qt::Key_F9},
    {"F10",      Qt::Key_F10},
    {"F11",      Qt::Key_F11},
    {"F12",      Qt::Key_F12}
    };

    QMap<QString, int> key;
    QString value;
    eKeyModes mode;

    command_key(const QString &val);
    bool is_valid()const;

    QStringList keys()const;
    int operator [](const QString &k);
    int operator [](const QString &k)const;

private:
    int spilt_value(const QString &keyval);

};


class command
{
public:
    const char* const KeySplit = "_";
    const struct
    {
        eControlModes type;
        const char *const value;
    } ControlModeValueList[Control_Count] =
    {
    {Control_All,   "All"},
    {Control_Timestamp,  "Timestamp"},
    {Control_Control, "Control"},
    {Control_State, "State"},
    {Control_Advanced, "Advanced"}
    };

    const struct
    {
        eLocalTypes type;
        const char *const value;
    } LocalTypeValueList[Sync_Count] =
    {
    {Sync_Not,   "Not"},
    {Sync_Host,  "Host"},
    {Sync_Slave, "Slave"}
    };

    const struct
    {
        eCommandLists cmdlist;
        const char *const keykey;
        const char *const netkey;
    }CommadKeyList[Command_Count]  =
    {
    /*  命令列表                键盘和本机配置key          网络同步配置key   */
    {Command_Url,             "Player/Url_1",           ""},// 横杠后面是跟序号，可以指定多个url
    {Command_Volume,          "Player/Volume",          ""},
    {Command_Mute,            "Player/Mute",            ""},

    {Command_Open,            "KeyCommand/Open",        "NetCommand/Open"},
    {Command_TooglePause,     "KeyCommand/TooglePause", "NetCommand/TooglePause"},
    {Command_TooglePlay,      "KeyCommand/TooglePlay",  "NetCommand/TooglePlay"},
    {Command_ToogleMute,      "KeyCommand/ToogleMute",  "NetCommand/ToogleMute"},
    {Command_AdjustVolume,    "KeyCommand/AdjustVolume","NetCommand/AdjustVolume"},
    {Command_Seek,            "KeyCommand/Seek",        "NetCommand/Seek"},
    {Command_ToogleFullScreen,"KeyCommand/FullScreen",  "NetCommand/FullScreen"},
    {Command_Quit,            "KeyCommand/Quit",        "NetCommand/Quit"},

    {Command_FullScreen,      "Display/FullScreen",     ""},
    {Command_Size,            "Display/Size",           ""},

    {Command_SyncHost,        "Sync/Host",              ""},
    {Command_LocalHost,       "Sync/Local",             ""},
    {Command_SyncMode,        "Sync/Control",           ""},
    {Command_SyncAutoDetect,  "Sync/AutoDetect",        ""},
    {Command_LocalType,       "Sync/Mode",              ""},
    {Command_SyncThreshold,   "Sync/Threshold",         ""}
    };

public:
    command(const QString &pf, QSettings::Format fm = QSettings::IniFormat);
    ~command();

    void update();

    //! 指定命令的value
    QVariant &operator [] (eCommandLists pc);
    QVariant operator [](eCommandLists pc)const;

    //! 指定命令的key
    QString key(eCommandLists pc, bool is_net = false)const;
    QStringList keys(eCommandLists pc, bool is_net = false)const;

    //! 指定key的value
    QVariant &operator [] (const QString &key);
    QVariant operator [](const QString &key)const;

    QList<int> event(eCommandLists cl, eKeyModes &mode);

    static command &self(){return *_this_;}
private:
    QSettings ini;
    static command *_this_;

    QMap<QString, QVariant> key_value;
};


#define cmdcfg command::self()

#endif // SYNC_CONFIG_H
