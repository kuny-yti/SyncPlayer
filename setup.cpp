#include "setup.h"

setup::setup(QWidget *parent) :
    QWidget(parent)
{
    ui = new Ui::setup();
    ui->setupUi(this);
}

setup::~setup()
{
    delete ui;
}
void setup::changeEvent(QEvent *e)
{
    QWidget::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void setup::on_but_save_config_clicked()
{
    emit sig_save_config();
}

void setup::on_but_fullscreen_clicked()
{
    emit sig_fullscreen();
}

void setup::on_but_exit_clicked()
{
    emit sig_exit();
}
