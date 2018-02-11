#ifndef SYNC_UDP_H
#define SYNC_UDP_H

#include <QThread>
#include <QUdpSocket>
#include "command.h"

namespace impl {
class udp_utils;
}
class sync_udp : public QThread
{
    Q_OBJECT
public:
    sync_udp(QObject *parent = 0);
    ~sync_udp();

signals:
    void sig_host_recv(const QByteArray &dat);
    void sig_slave_recv(const QByteArray &dat);

public slots:
    void on_local_mode(eLocalTypes lt);

    void on_send(const QByteArray &dat, const QHostAddress &addr, int port);
private slots:
    void on_try_recv();
    void on_try_disconnected();
    void on_try_connected();
protected:
    void run();
private:
    impl::udp_utils *impl;
};

#endif // SYNC_UDP_H
