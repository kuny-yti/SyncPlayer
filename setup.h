#ifndef SETUP_H
#define SETUP_H

#include <QWidget>
#include "ui_setup.h"

class setup : public QWidget
{
    Q_OBJECT
public:
    explicit setup(QWidget *parent = 0);
    ~setup();

signals:
    void sig_save_config();
    void sig_fullscreen();
    void sig_exit();

protected:
    void changeEvent(QEvent *e);

private slots:
    void on_but_save_config_clicked();
    void on_but_fullscreen_clicked();
    void on_but_exit_clicked();

private:
    Ui::setup *ui;
};

#endif // SETUP_H
