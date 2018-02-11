#include "player_ui.h"
#include <QKeyEvent>
#include <QTimer>
#include <QOpenGLWidget>

#include "command.h"
#include <QDesktopWidget>
#include <QApplication>
#include <QFileDialog>
#include <QStringList>

static const QRect toDesktopGeometry()
{
    uint screenNumber = 0;
    uint screenHeightMax = 0;
    uint screenHeightMaxBak = 0;

    QRect rect;
    QDesktopWidget *desktopWidget = QApplication::desktop();
    if (desktopWidget->screenCount())
       screenNumber =  desktopWidget->screenCount();
    else
        return QRect();

    for (uint i = 0; i < screenNumber; i++)
    {
        screenHeightMax = desktopWidget->screenGeometry(i).height();
        if (screenHeightMax > screenHeightMaxBak)
        {
            screenHeightMaxBak = screenHeightMax;
            rect.setHeight(screenHeightMax);
        }
        rect.setWidth(desktopWidget->screenGeometry(i).width() + rect.width());
    }
    return rect;
}

player_ui::player_ui(QWidget *parent) :
    QWidget(parent, Qt::FramelessWindowHint)
{
    vo_wid = NULL;
    vo_stretch = 1;
    mouse_press = false;
    fullscreen = false;

    main_layout = new QVBoxLayout(this);
    this->setLayout (main_layout);
    main_layout->setMargin (0);
    main_layout->setSpacing (1);

    ctrl_layout = new QHBoxLayout();
    ctrl_layout->setMargin (0);
    ctrl_layout->setSpacing (1);

    back_wid = new QWidget();
    back_wid->setLayout (ctrl_layout);

    main_layout->addWidget (back_wid, 0, 0);
    main_layout->setStretch(0, 1);
}
player_ui::~player_ui()
{
    delete ctrl_layout;
    delete main_layout;
}

void player_ui::install(QWidget *ctrl, int stretch)
{
    ctrl_layout->addWidget (ctrl);
    ctrl_layout->setStretch(ctrl_layout->count()-1, stretch);
}
void player_ui::uninstall(QWidget *ctrl)
{
    ctrl_layout->removeWidget (ctrl);
}
void player_ui::install(QOpenGLWidget *vo, int stretch)
{
    main_layout->removeWidget(back_wid);

    vo_wid = (QWidget*)vo;
    vo_stretch = stretch;
    main_layout->addWidget (vo_wid, 0);
    main_layout->setStretch(0, vo_stretch);

    main_layout->addWidget(back_wid, 1, 0);
    main_layout->setStretch(1, 1);
}
void player_ui::uninstall(QOpenGLWidget *vo)
{
    main_layout->removeWidget(vo);
}

void player_ui::on_fullscreen()
{
    if (!fullscreen)
    {
        back_wid->hide ();
        main_layout->removeWidget (back_wid);
        main_layout->removeWidget(vo_wid);

        main_layout->addWidget(vo_wid);
        main_layout->setStretch(0, 1);

        back_rect = this->geometry ();
        this->setGeometry (toDesktopGeometry());
        fullscreen = true;
    }
    else
    {
        back_wid->show ();
        main_layout->addWidget (back_wid, 1, 0);
        main_layout->setStretch(0, vo_stretch);
        main_layout->setStretch(1, 1);

        this->setGeometry (back_rect);
        fullscreen = false;
    }
}
void player_ui::on_key_event(QKeyEvent *ke)
{
    keyPressEvent(ke);
}
/// return > 0 加，== 0 无值， < 0 减
int press(eCommandLists cl, QKeyEvent *ke)
{
    eKeyModes keysmode  = Key_NotMode;
    QList<int> keys = cmdcfg.event(cl, keysmode);

    bool is_mult = true;
    int  is_key = 0;
    for (int i = 0; i < keys.count(); ++i)
    {
        if (keysmode == Key_ModifierMode)
        {
            if (keys[i] == ke->modifiers())
                is_mult &= true;
            else if (keys[i] == ke->key())
                is_key = 1;
        }
        else if (keysmode == Key_OrMode)
        {
            if (keys[i] == ke->modifiers())
                is_mult &= true;
            else if (keys[i] == ke->key())
                is_key  = 1;
        }
        else if (keysmode == Key_AndMode)
        {
            if (keys[i] == ke->modifiers())
                is_mult &= true;
            else if (keys[i] == ke->key())
                is_key  = i ? 1 : -1;
        }
        else if (keysmode == Key_SingleMode)
        {
            if (keys[i] == ke->key())
                is_key  = 1;
        }
    }
    return is_key;
}

void player_ui::keyPressEvent(QKeyEvent *ke)
{
    if (press(Command_ToogleFullScreen, ke))
        on_fullscreen();
    else if (press(Command_Quit, ke))
        emit sig_command(Command_Quit);
    else if (press(Command_Open, ke))
    {
        QString file =
                QFileDialog::getOpenFileName(this, tr("Select media file"),
                                             QString() ,video_stl, 0, QFileDialog::DontUseNativeDialog);
        if (!file.isEmpty())
            sig_open_movie(file);
    }
    else if (press(Command_TooglePause, ke))
        emit sig_command(Command_TooglePause);
    else if (press(Command_TooglePlay, ke))
        emit sig_command(Command_TooglePlay);
    else if (press(Command_AdjustVolume, ke) > 0)
        emit sig_adjust_volume(0.1);
    else if (press(Command_AdjustVolume, ke) < 0)
        emit sig_adjust_volume(-0.1);
    else if (press(Command_ToogleMute, ke))
        emit sig_command(Command_ToogleMute);
    else if (press(Command_Seek, ke) > 0)
        emit sig_seek_movie(0.001);
    else if (press(Command_Seek, ke) < 0)
        emit sig_seek_movie(-0.001);
}

void player_ui::mousePressEvent(QMouseEvent *event)
{
    static qint64 pets = 0;
    static bool has_press = cmdcfg[Command_LocalType].toString() == cmdcfg.LocalTypeValueList[Sync_Host].value;
    if(event->button() == Qt::RightButton)
    {
        if (fullscreen)
            return;
        mouse_press = true;
        move_point = event->pos();
    }
    else if(event->button() == Qt::LeftButton && has_press && ((get_microseconds () - pets) > 1000*1000*10))
    {
        emit sig_switch_movie();
        pets = get_microseconds ();
    }
}
void player_ui::mouseMoveEvent(QMouseEvent *event)
{
    if(mouse_press)
    {
        QPoint move_pos = event->globalPos();

        this->move(move_pos - move_point);
    }
}
void player_ui::mouseReleaseEvent(QMouseEvent *event)
{
    mouse_press = false;
}
