#ifndef TIMELISTENER_H
#define TIMELISTENER_H
#include <QThread>
#include <QHostAddress>
class TimeListener: public QObject
{
    Q_OBJECT
public:
    enum Type{
        undef = 0,
        Multicast = 1,
        Tcp = 2,
    };
    TimeListener(QString ipport);
    static TimeListener * fromType(Type t, QString connection);
    virtual ~TimeListener() = default;
    virtual void startSinchronize() = 0;
protected:
    bool parsed = false;
    QString sAddr;
    quint16 port{0};
};

#include <QUdpSocket>
class MulticastTimeListener : public TimeListener
{
    Q_OBJECT
public:
    MulticastTimeListener(QString ipport);
    ~MulticastTimeListener();
    void startSinchronize();

private Q_SLOTS:
    void readDatagramm();

private:
    QUdpSocket sock;
};

#include <memory>
#include "clienttcp.h"
class TcpTimeListener : public TimeListener
{
    Q_OBJECT
public:
    TcpTimeListener(QString ipport);
    ~TcpTimeListener();
    void startSinchronize();

private Q_SLOTS:
    void connected(ClientTcp * );
    void disconnected(ClientTcp * );
    void readyread(ClientTcp * );

private:

    std::shared_ptr<ClientTcp> client;
};

#endif // TIMELISTENER_H
