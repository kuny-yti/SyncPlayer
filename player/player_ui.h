#ifndef PLAYER_UI_H
#define PLAYER_UI_H
#include <QWidget>
#include "QGridLayout"
#include "QHBoxLayout"

class player_ui : public QWidget
{
    Q_OBJECT
public:
    explicit player_ui(QWidget *parent = 0);
    ~player_ui();

    bool is_fullscreen()const{return fullscreen;}
    // 安装控制界面
    void install(QWidget *ctrl, int stretch);
    void uninstall(QWidget *ctrl);

    // 安装输出界面
    void install(class QOpenGLWidget *vo, int stretch = 4);
    void uninstall(class QOpenGLWidget *vo);

    void has_switch_movie(bool b = false);
signals:
    void sig_switch_movie();
    void sig_adjust_volume(float val);
    void sig_seek_movie(float val);
    void sig_open_movie(const QString &url);
    void sig_command(int cmd);

    // 全屏
public slots:
    void on_fullscreen();
    void on_key_event(class QKeyEvent *);

protected:
    virtual void keyPressEvent(QKeyEvent *);
    virtual void mousePressEvent(QMouseEvent *event);
    virtual void mouseReleaseEvent(QMouseEvent *event);
    virtual void mouseMoveEvent(QMouseEvent *event);

private:
    QHBoxLayout* ctrl_layout;
    QVBoxLayout* main_layout;

    bool fullscreen;

    QPoint move_point; //移动的距离
    bool mouse_press; //鼠标按下

    QWidget *back_wid;
    QRect back_rect;

    QWidget *vo_wid;
    int vo_stretch;
};

#endif // PLAYER_UI_H
