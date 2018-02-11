#ifndef SYNC_TCP_H
#define SYNC_TCP_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>
#include <QTimer>
#include "command.h"

namespace impl {

class client :public QThread
{
    Q_OBJECT
public:
    client(int Handle = -1, QObject *parent = NULL);
    ~client();
    QString addr()const;
signals:
    void sig_recv(const QByteArray & ba);
    void sig_connect(bool, const QString&);
public slots:
    void on_send(const QByteArray &ba);
public:
    bool connected(const QHostAddress &ha, int p=998);
protected:
    void run();

private slots:
    void on_recv();
    void on_connected();
    void on_disconnected();

    void on_tryconnect();
private:
    QTcpSocket *sock;
    QString sock_addr;
    QHostAddress addrs;
    int port;
    QTimer *try_connect;
};

class server :public QTcpServer
{
    Q_OBJECT
public:
    server(qint16 port = 998, QObject *parent = NULL);
    ~server();

signals:
    void sig_recv(const QByteArray &ba);
    void sig_connect(bool, const QString&);

public slots:
    void on_send(const QByteArray &ba);
    void on_send(const QByteArray &ba, const QHostAddress &addr);

private slots:
    void on_connected(bool s, const QString&addr);
protected:
    void incomingConnection(int handle);
private:
    QMap<QString, client *> map_socket;
};

class tcp_utils;
}
class sync_tcp : public QThread
{
    Q_OBJECT
public:
    sync_tcp(QObject *parent = 0);
    ~sync_tcp();

signals:
    void sig_host_recv(const QByteArray &dat);
    void sig_slave_recv(const QByteArray &dat);
    void sig_slave_list(bool, const QString &addr);

public slots:
    void on_local_mode(eLocalTypes lt, const QHostAddress &ser);

    void on_send(const QByteArray &dat, const QHostAddress &addr, int port);
    void on_exit();
private slots:
    void on_recv(const QByteArray &ba);

protected:
    void run();

private:
    impl::tcp_utils *impl;
};
#endif // SYNC_TCP_H
