#ifndef CTRL_SYNC_H
#define CTRL_SYNC_H

#include "ui_ctrl_sync.h"
#include "command.h"
#include "sync_data.h"
#include <QTimer>
#include <QHostAddress>

class ctrl_sync : public QWidget, private Ui::ctrl_sync
{
    Q_OBJECT
public:
    explicit ctrl_sync(QWidget *parent = 0);
    ~ctrl_sync();

signals:
    void sig_local_type(eLocalTypes);
    void sig_local_mode(eLocalTypes, const QHostAddress &ad);
    void sig_sync_threshold(int val);
    void sig_sync_mode(eSyncModes);
    void sig_auto_dectect(bool );

public slots:
    void on_update_cfg(command &cmd);

    //void on_ping_host(const tPingReplys &pr);   // 从机-ping主机
    void on_slave_list(bool s, const QString &addr);// 主机-所有从机地址列表
    void on_online_host(bool s);                // 从机-主机是否在线

protected:
    void changeEvent(QEvent *e);

private slots:
    // 脱机模式
    void on_but_not_toggled(bool checked);
    // 从机模式
    void on_but_slave_toggled(bool checked);
    // 主机模式
    void on_but_host_toggled(bool checked);

    void on_ping_slave();                       // 主机-ping所有从机
    void on_spinBox_valueChanged(int arg1);

    void on_cb_control_stateChanged(int arg1);

    void on_cb_timestamp_stateChanged(int arg1);

    void on_cb_dectect_play_stateChanged(int arg1);

private:
    //net_utils network;
    QTimer timer;
};

#endif // CTRL_SYNC_H
