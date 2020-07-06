#include "timelistener.h"
#include <QDateTime>
#include <QTimer>
#include <QRegularExpression>

#define dtmFmt "dd.MM.yyyy hh:mm:ss"

bool ParseIpPort(QString ipport, QString &ip, quint16 &port)
{
    bool ret = true;
    ip.clear();
    port = 0;
    QRegularExpressionMatch match = QRegularExpression("\\b([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3})\\.([0-9]{1,3}):[0-9]{1,5}\\b").match(ipport);
    if (match.hasMatch())
    {
        // проблема: обработка рекурсивных подключений, например: 127.0.0.1:28080/192.168.0.101:28080
        QString lexem = match.captured();
        QRegularExpressionMatch matchip   = QRegularExpression(".+(?=:)").match(lexem);
        QRegularExpressionMatch matchport = QRegularExpression("(?<=:).+").match(lexem);
        // QRegularExpressionMatch matchport2 = QRegularExpression("\\d+$").match(ipport);
        if (matchip.hasMatch())
            ip = matchip.captured();
        else
            ret = false;
        if (matchport.hasMatch())
            port = matchport.captured().toInt();
        else
            ret = false;
    }
    else
        ret = false;
    return ret;
}


TimeListener::TimeListener(QString ipport)
{
    parsed = ParseIpPort(ipport,sAddr,port);
    Logger::LogStr(parsed ? QString("Успешный разбор строки соединения " + ipport): QString("Ошибка разбора строки соединения " + ipport));
}

TimeListener *TimeListener::fromType(TimeListener::Type t, QString connection)
{
    if(t == Multicast)
        return new MulticastTimeListener(connection);
    else if (t == Tcp)
        return new TcpTimeListener(connection);
    else
        throw std::logic_error("Неудовлетворительный тип соединения");
}

MulticastTimeListener::MulticastTimeListener(QString ipport)
    : TimeListener(ipport)
{

}

MulticastTimeListener::~MulticastTimeListener()
{
    sock.leaveMulticastGroup(QHostAddress(sAddr));
}

void MulticastTimeListener::startSinchronize()
{
    if(!parsed)
        return;

    QObject::connect(&sock, &QUdpSocket::readyRead, this, &MulticastTimeListener::readDatagramm);
    sock.bind(QHostAddress(sAddr),port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if(sock.joinMulticastGroup(QHostAddress(sAddr)))
    {
        Logger::LogStr(QString("MulticastTimeListener: Подключен к группе %1:%2").arg(sAddr).arg(port));
    } else {
        Logger::LogStr(QString("MulticastTimeListener: Ошибка подключения к группе %1:%2").arg(sAddr).arg(port));
    }
}

void MulticastTimeListener::readDatagramm()
{
    QHostAddress senderHost;
    quint16 senderPort;
    quint64 size = sock.pendingDatagramSize();
    QByteArray datagram;
    // читаем че пришло
    datagram.resize(size);
    sock.readDatagram(datagram.data(), size, &senderHost, &senderPort);
    datagram.remove(0,4);

    QString sDateTime(datagram);

    // берем метку времени
    time_t tm = QDateTime::fromString(sDateTime, dtmFmt).toTime_t();

    // меняем время
#ifndef DEBUG
    stime(&tm);
#else
    sDateTime = QDateTime::fromTime_t(tm).toString(dtmFmt);
#endif

    qDebug()<<datagram;
    Logger::LogStr(QString("Произведена синхронизация по Multicast-соединению: %1 на время %2")
                   .arg(senderHost.toString() + ":" + QString::number(senderPort))
                   .arg(sDateTime));
}




TcpTimeListener::TcpTimeListener(QString ipport)
    :TimeListener(ipport)
{

}

TcpTimeListener::~TcpTimeListener()
{
    if(client)
        client->disconnect();
}

void TcpTimeListener::startSinchronize()
{
    if(!parsed)
        return;

    client = std::shared_ptr<ClientTcp>(new ClientTcp(sAddr, port));
    client->setAutoRecconect(true);
    QObject::connect(client.get(), SIGNAL(connected    (ClientTcp*)), this, SLOT(connected      (ClientTcp*)));
    QObject::connect(client.get(), SIGNAL(dataready    (ClientTcp*)), this, SLOT(dataredy       (ClientTcp*)));
    QObject::connect(client.get(), SIGNAL(disconnected (ClientTcp*)), this, SLOT(disconnected   (ClientTcp*)));
    QObject::connect(client.get(), SIGNAL(error        (ClientTcp*)), this, SLOT(error          (ClientTcp*)));
    client->start();
}

void TcpTimeListener::connected(ClientTcp * conn)
{
    Logger::LogStr(tr("TcpTimeListener: Подключен к %1").arg(conn->name()));
}

void TcpTimeListener::disconnected(ClientTcp * conn)
{
    Logger::LogStr(tr("TcpTimeListener: Отключен от %1").arg(conn->remoteIP()));
}

void TcpTimeListener::readyread(ClientTcp * conn)
{
    QString sDateTime(conn->data());
    time_t tm = QDateTime::fromString(sDateTime, dtmFmt).toTime_t();

#ifndef DEBUG
    stime(&tm);
#else
    sDateTime = QDateTime::fromTime_t(tm).toString();
#endif

    Logger::LogStr(QString("Произведена синхронизация по Tcp-соединению: %1 на время %2")
                   .arg(conn->name())
                   .arg(sDateTime));
}



