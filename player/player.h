#ifndef PLAYER_H
#define PLAYER_H
#include "type.h"
#include "video_frame.h"
#include "subtitle_box.h"
#include "open_input_data.h"
#include "media_input.h"
#include <QObject>
#include "video_output.h"
#include "audio_output.h"

typedef void (*update_ts)(qint64 progress, double pos);
typedef void (*ctrl_command)(int cmd);
namespace impl {
class players;
}
class player
{
    friend class player_thread;
private:
    impl::players *impl;

public:
    player(video_output *vo, audio_output *ao = NULL);
    virtual ~player();

    //![0]
    void playlist(const QStringList &list_path, bool clear = true);//设置播放器列表
    QStringList playlist();//返回播放器列表

    void curren_file(const QString&);//设置当前播放的文件
    QString curren_file();//当前播放的文件
    //![0]

    //![1]
    void open(const QString&file = QString());//打开播放器
    void close();//关闭播放器
    //![1]

    //![2]
    void play(bool s = true);//开始播放
    void pause(bool s = true);//暂停
    void seek(double val);//移动位置
    void seek(qint64 val);//移动位置
    //![2]

    //![3]
    void mute(bool val = true);//静音
    void volume(float val);//设置音量
    float volume();//返回当前音量
    //![3]

    //![3]
    QImage capture(); //截图
    //![3]

    //![4]
    bool is_mute(); //是否静音
    bool is_play(); //是否播放
    bool is_pause();//是否暂停
    bool is_open();
    //![4]

    qint64 duration()const;// 总长度
    double pos()const;// 当前相对进度
    qint64 tell()const; // 当前绝对进度
    qint64 frame()const;// 当前视频帧计数
    float frame_rate()const;
    void set(update_ts fun);
    void loop(eLoopMode);
    bool playend()const;

private:
    bool run_step();

public :
    void toggle_play();//若在播放状态则停止
    void toggle_pause();//若在播放状态则暂停
    void toggle_mute();//若是静音则设置开启

    void lock();
    void unlock();
    void logs(const QString &text);
};

#endif // PLAYER_H
