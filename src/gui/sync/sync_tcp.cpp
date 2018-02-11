#include "sync_tcp.h"
#include <QTcpServer>
#include <QMap>
#include <QTimer>

namespace impl {

client::client(int Handle, QObject *parent):
    QThread(parent)
{
    try_connect = NULL;
    sock = new QTcpSocket(this);
    if (Handle > 0)
        sock->setSocketDescriptor(Handle);

    connect(sock, &QTcpSocket::readyRead, this, &client::on_recv);
    connect(sock, &QTcpSocket::connected, this, &client::on_connected);
    connect(sock, &QTcpSocket::disconnected, this, &client::on_disconnected);

    this->start ();
    sock_addr = sock->peerAddress().toString();
}
client::~client()
{
    if (try_connect)
        delete try_connect;
    try_connect = NULL;
    sock->close();
    this->wait(1000);
    this->quit();
    delete sock;
}
QString client::addr()const
{
    return sock_addr;
}
void client::on_send(const QByteArray &ba)
{
    if (sock->isOpen ())
        sock->write (ba);
}
bool client::connected(const QHostAddress &ha, int p)
{
    addrs = ha;
    port = p;
    sock->connectToHost (ha, p);
    int ret = sock->waitForConnected(1000);

    if (!try_connect)
    {
        try_connect = new QTimer(this);
        connect (try_connect, &QTimer::timeout, this, &client::on_tryconnect);
    }

    if (sock->state () != QAbstractSocket::ConnectedState || ret >= 1000)
    {
        if (!try_connect->isActive ())
            try_connect->start (5*1000);
        return false;
    }

    if (try_connect && try_connect->isActive ())
        try_connect->stop ();

    return true;
}
void  client::on_tryconnect()
{
    if (connected(addrs, port))
    {
    }
}
void client::run()
{
    this->exec();
}

void client::on_recv()
{
    emit sig_recv(sock->readAll ());
}
void client::on_connected()
{
    sock_addr = sock->peerAddress().toString();
    emit sig_connect (true, sock_addr);
}
void client::on_disconnected()
{
    if (try_connect&& !try_connect->isActive ())
        try_connect->start (5*1000);
    emit sig_connect (false, sock_addr);
    //sock->close();
}

server::server(qint16 port, QObject *parent):
    QTcpServer(parent)
{
    this->listen (QHostAddress::Any, port);
}
server::~server()
{
    QList<client *> socklist = map_socket.values ();
    for (int i = 0; i < socklist.size (); ++i)
        delete socklist[i];//线程对象还存在，则不会自动删除，需要手动删除
    map_socket.clear();
    this->close ();
}
void server::on_send(const QByteArray &ba)
{
    QList<client *> socklist = map_socket.values ();
    for (int i = 0; i < socklist.size (); ++i)
        if (socklist[i])
            socklist[i]->on_send (ba);
}
void server::on_send(const QByteArray &ba, const QHostAddress &addr)
{
    QList<client *> socklist = map_socket.values ();
    for (int i = 0; i < socklist.size (); ++i)
        if (socklist[i] &&socklist[i]->addr () == addr.toString ())
            socklist[i]->on_send (ba);
}

void server::on_connected(bool s, const QString&addr)
{
    if (!s)
        map_socket.remove (addr);
    emit sig_connect(s, addr);
}
void server::incomingConnection(int handle)
{
    client* thread = new client(handle, this);

    this->connect(thread, &client::finished, thread, &client::deleteLater);//线程执行完成后删除对象

    this->connect(thread, &client::sig_recv, this, &server::sig_recv);
    this->connect(thread, &client::sig_connect, this, &server::on_connected);

    emit on_connected(true, thread->addr ());

    map_socket.insert (thread->addr (), thread);
}

class tcp_utils
{
public:
    eLocalTypes local_mode;
    client     *tcp_socket;
    server     *tcp_server;

    tcp_utils()
    {
        tcp_socket = NULL;
        tcp_server = NULL;
        local_mode = Sync_Not;
    }
    ~tcp_utils()
    {

    }
};
}

sync_tcp::sync_tcp(QObject *parent):
    QThread(parent)
{
    impl = new impl::tcp_utils();

}
sync_tcp::~sync_tcp()
{
    if (impl->tcp_server)
        delete impl->tcp_server;
    if (impl->tcp_socket)
        delete impl->tcp_socket;
    delete impl;
}
void sync_tcp::on_exit()
{
    on_local_mode(Sync_Not, QHostAddress());
}

void sync_tcp::on_send(const QByteArray &dat, const QHostAddress &addr, int)
{
    if (impl->local_mode == Sync_Host &&impl->tcp_server)
    {
        if (addr.toString ().isEmpty ())
            impl->tcp_server->on_send (dat);
        else
            impl->tcp_server->on_send (dat, addr);
    }
    else if (impl->local_mode == Sync_Slave && impl->tcp_socket)
        impl->tcp_socket->on_send (dat);
}

void sync_tcp::on_recv(const QByteArray &dat)
{
    if (impl->local_mode == Sync_Host)
        emit sig_host_recv (dat);
    else if (impl->local_mode == Sync_Slave && impl->tcp_socket)
        emit sig_slave_recv (dat);
}
void sync_tcp::on_local_mode(eLocalTypes lt, const QHostAddress &ser)
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
        if (impl->tcp_server)
        {
            delete impl->tcp_server;
            impl->tcp_server = NULL;
        }
        if (impl->tcp_socket)
        {
            delete impl->tcp_socket;
            impl->tcp_socket = NULL;
        }
        impl->tcp_server = new impl::server();
        connect (impl->tcp_server, &impl::server::sig_recv, this, &sync_tcp::on_recv);
        connect (impl->tcp_server, &impl::server::sig_connect, this, &sync_tcp::sig_slave_list);
    }
    else if (impl->local_mode == Sync_Slave)
    {
        if (impl->tcp_server)
        {
            delete impl->tcp_server;
            impl->tcp_server = NULL;
        }
        if (impl->tcp_socket)
        {
            delete impl->tcp_socket;
            impl->tcp_socket = NULL;
        }

        impl->tcp_socket = new impl::client();
        connect (impl->tcp_socket, &impl::client::sig_recv, this, &sync_tcp::on_recv);
        connect (impl->tcp_socket, &impl::client::sig_connect, this, &sync_tcp::sig_connect);
        impl->tcp_socket->connected (ser, 998) ;
        cmdcfg[Command_LocalHost] =impl->tcp_socket->addr ();
    }
    else
    {
        if (impl->tcp_server)
        {
            delete impl->tcp_server;
            impl->tcp_server = NULL;
        }
        if (impl->tcp_socket)
        {
            delete impl->tcp_socket;
            impl->tcp_socket = NULL;
        }
    }
}

void sync_tcp::run()
{
    this->exec ();
}
