#include "ctrl_default.h"
#include <QFileDialog>
#include "command.h"
#include "player_core.h"

ctrl_default::ctrl_default(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);
    time_value_t=  0;
}
ctrl_default::~ctrl_default()
{
}

void ctrl_default::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        retranslateUi(this);
        break;
    default:
        break;
    }
}
void ctrl_default::on_update_ts(qint64 duration, double pos, qint64 )
{
    time_value_t = pos * this->slider_time->maximum();
    this->slider_time->setValue(time_value_t);

    qint64 ms = (duration / 1000) % 1000;
    qint64 s = (duration / 1000 /1000) % 60;
    qint64 m = (duration / 1000 /1000 /60) % 60;
    qint64 h = (duration / 1000 /1000 /60 /60) % 24;

    QString str_time = QString("%1:%2:%3:%4").arg(h).arg(m).arg(s).arg(ms);
    this->label_time->setText(str_time);
}
void ctrl_default::on_update_cfg(command &cmd)
{
    if (cmdcfg[Command_LocalType].toString() != cmdcfg.LocalTypeValueList[Sync_Host].value)
        return;
    if (!cmd[Command_Url].toString().isEmpty())
    {
        text_play_list->clear();
        text_play_list->setText (cmd[Command_Url].toString());
    }
    if (cmd[Command_Volume].toFloat() > 0.0)
        slider_volum->setValue(cmd[Command_Volume].toFloat() * 1000);
}
void ctrl_default::on_sync(const QByteArray &data, const QHostAddress &, int )
{
    QByteArray dats = data;
    tSyncDatas sd(dats);
    switch ((int)sd.cmd) {
    case Sync_SeekTo:
        slider_time->setValue(this->slider_time->maximum() * sd.seek);
        break;
    case Sync_AdjustVolum:
        slider_volum->setValue(sd.volume * 1000);
        break;
    case Sync_SwitchMovie:
        text_play_list->clear();
        text_play_list->setText (sd.url);
        break;
    default:
        break;
    }
}
void ctrl_default::on_local_type(eLocalTypes lt)
{
    if (lt == Sync_Slave)
        this->setHidden(true);
    else
        this->setHidden(false);
}
void ctrl_default::on_bt_open_clicked()
{
    QString url = QFileDialog::getOpenFileName(this, tr("Select media file"),
                                                QString() ,video_stl, 0, QFileDialog::DontUseNativeDialog);
    if (url.isEmpty())
        return;

    emit sig_open_movie(url);
}

void ctrl_default::on_bt_play_clicked()
{
    emit sig_command(Command_TooglePlay) ;
}

void ctrl_default::on_bt_pause_clicked()
{
     emit sig_command(Command_TooglePause) ;
}

void ctrl_default::on_bt_mute_clicked()
{
    emit sig_command(Command_ToogleMute) ;
}

void ctrl_default::on_slider_volum_valueChanged(int value)
{
    emit sig_volume(float(value / 1000.0));
}

void ctrl_default::on_slider_time_valueChanged(int value)
{
    if (time_value_t != value)
    {
        emit sig_seek((float)value / (float)this->slider_time->maximum());
        time_value_t = value;
    }
}
