#ifndef SYNC_CORE_H
#define SYNC_CORE_H

#include <QObject>
#include <QMutex>

#define coreLogsWrite sync_core::logsw

namespace impl {
class core;
}
class sync_core :public QObject
{
    Q_OBJECT
public:
    sync_core(QObject *parent=0);
    ~sync_core();

    static void logsw(const QString &str);
public slots:
    void on_update_pos(double); // 更新播放进度
    void on_update_length(double v); // 更新影片的长度
    void on_switch_movie(); // 触控切换影片

signals:
    void sig_view(); // 精简视图
    void sig_full(); // 全屏
    void sig_playlist(const QStringList &lis); // 设置播放列表
    void sig_select(const QString &movie); // 选择需要播放的影片
    void sig_seek(int);

private slots:
    void on_host_recv(const QByteArray &ba);// 主机接收数据
    void on_slave_recv(const QByteArray &ba);// 从机接收数据
    void on_disconnect(bool , const QString &);
    void on_detect();
    void on_loadcfg(); // 加载配置

private:
    impl::core *impl;
    static QMutex logsm;
};

#endif // SYNC_CORE_H
