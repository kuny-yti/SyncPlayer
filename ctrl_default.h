#ifndef CTRL_DEFAULT_H
#define CTRL_DEFAULT_H

#include "ui_ctrl_default.h"
#include "command.h"
#include "sync_data.h"

#include <QHostAddress>

class ctrl_default : public QWidget, private Ui::ctrl_default
{
    Q_OBJECT
public:
    explicit ctrl_default(QWidget *parent = 0);
    ~ctrl_default();

signals:
    void sig_open_movie(const QString &url);
    void sig_command(int cmd);
    void sig_volume(float vol);
    void sig_seek(float pos);

public slots:
    void on_update_cfg(command &cmd);
    void on_update_ts(qint64 duration, double pos, qint64 dur);
    void on_sync(const QByteArray &data, const QHostAddress &addr, int port);
    void on_local_type(eLocalTypes);

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_bt_open_clicked();
    void on_bt_play_clicked();
    void on_bt_pause_clicked();
    void on_bt_mute_clicked();
    void on_slider_volum_valueChanged(int value);
    void on_slider_time_valueChanged(int value);

private:
    int time_value_t;
};

#endif // CTRL_DEFAULT_H
