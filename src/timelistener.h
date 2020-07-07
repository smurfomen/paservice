#ifndef TIMELISTENER_H
#define TIMELISTENER_H
#include <QThread>
#include <QHostAddress>
#include <memory>

class TimeListener: public QObject
{
    Q_OBJECT
public:
    TimeListener(QString ipport);
    static std::shared_ptr<TimeListener> fromType(QString t, QString connection);
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
    void dataready(ClientTcp * );

private:
    std::shared_ptr<ClientTcp> client;
};

#endif // TIMELISTENER_H
