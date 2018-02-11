#include "ctrl_sync.h"
#include "command.h"


ctrl_sync::~ctrl_sync()
{
}
ctrl_sync::ctrl_sync(QWidget *parent) :
    QWidget(parent)
{
    setupUi(this);

    this->group_host->hide ();
    this->group_slave->hide ();

    connect (&timer, &QTimer::timeout, this, &ctrl_sync::on_ping_slave);
}
void ctrl_sync::on_update_cfg(command &cmd)
{
    if (!cmd[Command_SyncHost].toString().isEmpty())
        this->edit_host->setText(cmd[Command_SyncHost].toString());

    if (cmd[Command_LocalType].toString().isEmpty() ||
        cmd[Command_LocalType].toString() == cmd.LocalTypeValueList[Sync_Not].value)
    {
        this->but_not->setChecked(true);
        on_but_not_toggled(true);
    }
    else if (cmd[Command_LocalType].toString() == cmd.LocalTypeValueList[Sync_Host].value)
    {
        this->but_host->setChecked(true);
        on_but_host_toggled(true);
    }
    else if (cmd[Command_LocalType].toString() == cmd.LocalTypeValueList[Sync_Slave].value)
    {
        this->but_slave->setChecked(true);
        on_but_slave_toggled(true);
    }
    this->spinBox->setValue (cmd[Command_SyncThreshold].toInt ());

    if (cmd[Command_SyncMode].toString () == cmd.ControlModeValueList[Control_All].value)
    {
        this->cb_control->setChecked (true);
        this->cb_timestamp->setChecked (true);
    }
    else if (cmd[Command_SyncMode].toString () == cmd.ControlModeValueList[Control_Control].value)
        this->cb_control->setChecked (true);
    else if (cmd[Command_SyncMode].toString () == cmd.ControlModeValueList[Control_Timestamp].value)
        this->cb_timestamp->setChecked (true);

}

void ctrl_sync::changeEvent(QEvent *e)
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

// 不启用同步-脱机模式(关闭同步)
void ctrl_sync::on_but_not_toggled(bool checked)
{
    if (checked)
    {
        emit sig_local_mode(Sync_Not, QHostAddress(edit_host->text ()));
        emit sig_local_type(Sync_Not);
        this->label_state->setText ("state: offline");
        this->label_ping->setText ("ping: not");
        timer.stop ();
        list_addr->setRowCount (0);

        this->group_host->hide ();
        this->group_slave->hide ();

        this->spinBox->show ();
        this->label_sync_ts->show ();
        this->cb_control->show ();
        this->cb_timestamp->show ();
        this->label_host_addr->show ();
        this->edit_host->show ();
        this->edit_host->setEnabled (true);
    }
}

// 本机为从机模式(要从host里面获取主机的地址)
void ctrl_sync::on_but_slave_toggled(bool checked)
{
    if (checked)
    {
        this->label_state->setText ("state: offline");
        this->label_ping->setText ("ping: not");
        timer.stop ();
        list_addr->setRowCount (0);

        this->group_host->show ();
        this->group_slave->hide ();

        this->label_host_addr->show ();
        this->edit_host->show ();
        this->edit_host->setEnabled (false);

        this->spinBox->hide ();
        this->label_sync_ts->hide ();
        this->cb_control->hide ();
        this->cb_timestamp->hide ();

        emit sig_local_mode(Sync_Slave, QHostAddress(edit_host->text ()));
        emit sig_local_type(Sync_Slave);
    }
}
// 本机为主机模式
void ctrl_sync::on_but_host_toggled(bool checked)
{
    if (checked)
    {
        this->label_state->setText ("state: offline");
        this->label_ping->setText ("ping: not");
        timer.start (500);

        this->group_host->hide ();
        this->group_slave->show ();
        this->spinBox->show ();
        this->label_sync_ts->show ();
        this->cb_control->show ();
        this->cb_timestamp->show ();

        this->edit_host->hide();
        this->label_host_addr->hide ();

        emit sig_local_mode(Sync_Host, QHostAddress(edit_host->text ()));
        emit sig_local_type(Sync_Host);
    }
}

void ctrl_sync::on_ping_slave()
{
    int count = list_addr->rowCount ();
    for (int i = 0; i < count; i++)
    {
        QTableWidgetItem * item = list_addr->item (i, 0);
        if (item)
        {
            if (!item->text ().isEmpty ())
            {
                QString addr = item->text ();
                //tPingReplys pr;
                //if (network.ping (addr.toLatin1 ().data (), &pr))
                //{
                //    QTableWidgetItem * item = list_addr->item (i, 1);
                //    if (item)
                //        item->setText (" "+QString::number (pr.timecost)+ " ");
                //}
            }
        }
    }
}
/*
void ctrl_sync::on_ping_host(const tPingReplys &pr)
{
    QString p;
    p.append ("ping: ");
    p.append (QString::number (pr.timecost));
    this->label_ping->setText (p);
}*/
void ctrl_sync::on_slave_list(bool s, const QString &addr)
{
    QList<QString> addrlist;
    for (int i = 0; i < list_addr->rowCount (); ++i)
        addrlist.append (list_addr->item (i, 0)->text ());

    if (s)
        addrlist.append (addr);
    else
        addrlist.removeOne (addr);

    list_addr->setRowCount (addrlist.size ());
    for (int i = 0; i < addrlist.size (); ++i)
    {
        if (addrlist[i] == addr && !s)
            continue;
        list_addr->setItem (i, 0, new QTableWidgetItem(addrlist[i]));
    }
}
void ctrl_sync::on_online_host(bool s)
{
    if(s)
        this->label_state->setText ("state: online");
    else
        this->label_state->setText ("state: offline");
}

void ctrl_sync::on_spinBox_valueChanged(int arg1)
{
    emit sig_sync_threshold(arg1);
}

void ctrl_sync::on_cb_control_stateChanged(int arg1)
{
    if (arg1 && this->cb_timestamp->isChecked ())
        emit sig_sync_mode (Sync_ModeAll);
    else if (arg1)
        emit sig_sync_mode (Sync_ModeContorl);
    else if (this->cb_timestamp->isChecked ())
        emit sig_sync_mode (Sync_ModeTimestamp);
}

void ctrl_sync::on_cb_timestamp_stateChanged(int arg1)
{
    if (arg1 && this->cb_control->isChecked ())
        emit sig_sync_mode (Sync_ModeAll);
    else if (arg1)
        emit sig_sync_mode (Sync_ModeTimestamp);
    else if (this->cb_control->isChecked ())
        emit sig_sync_mode (Sync_ModeContorl);
}

// 自动检测在10秒未播放时切换影片
void ctrl_sync::on_cb_dectect_play_stateChanged(int arg1)
{
    emit sig_auto_dectect( arg1 > 0 ? true :false);
}
