#include "sync_udp.h"
#include <QUdpSocket>

namespace impl {
class udp_utils
{
public:
    eLocalTypes local_mode;
    QUdpSocket *udp_socket;

    udp_utils()
    {
        local_mode = Sync_Not;
    }
    ~udp_utils()
    {

    }
};
}

sync_udp::sync_udp(QObject *parent):
    QThread(parent)
{
    impl = new impl::udp_utils();
    impl->udp_socket = new QUdpSocket(this);
    connect (impl->udp_socket, &QUdpSocket::readyRead, this, &sync_udp::on_try_recv);
    connect (impl->udp_socket, &QUdpSocket::disconnected, this, &sync_udp::on_try_disconnected);
    connect (impl->udp_socket, &QUdpSocket::connected, this, &sync_udp::on_try_connected );
}
sync_udp::~sync_udp()
{
    delete impl->udp_socket;
    delete impl;
}
void sync_udp::on_send(const QByteArray &dat, const QHostAddress &addr, int port)
{
    if (impl->local_mode == Sync_Host)
        impl->udp_socket->writeDatagram (dat, addr, port);
}

void sync_udp::on_try_disconnected()
{

}
void sync_udp::on_try_connected()
{

}
void sync_udp::on_try_recv()
{
    while(impl->udp_socket->hasPendingDatagrams())
    {
        QHostAddress host;
        quint16 port;
        QByteArray rcv_buf;
        rcv_buf.resize (impl->udp_socket->pendingDatagramSize());
        impl->udp_socket->readDatagram(rcv_buf.data (), rcv_buf.size (), &host, &port);
        if (impl->local_mode == Sync_Host)
            emit sig_host_recv(rcv_buf);
        else if (impl->local_mode == Sync_Slave)
            emit sig_slave_recv(rcv_buf);
    }
}
void sync_udp::on_local_mode(eLocalTypes lt)
{
    if (lt == impl->local_mode)
        return ;
    impl->local_mode = lt;

    if (lt == Sync_Not || this->isRunning())
        this->terminate ();
    else if (lt != Sync_Not && !this->isRunning())
        this->start ();

    if (impl->local_mode == Sync_Host)
    {
        impl->udp_socket->close ();
    }
    else if (impl->local_mode == Sync_Slave)
    {
        impl->udp_socket->close ();
        impl->udp_socket->bind (999);
    }
    else
        impl->udp_socket->close ();
}

void sync_udp::run()
{
    this->exec ();
}
