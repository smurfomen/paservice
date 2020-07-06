#ifndef SERVERTCP_H
#define SERVERTCP_H

/*
 * Класс ServerTcp используется для организации Tcp-сервера в технологии ПО ДЦ-ЮГ
 * Порядок использования класса
 * - включить в проект файлы:
 *   - servertcp.cpp, servertcp.h
 *   - clienttcp.cpp, clienttcp.h
 *   - tcpheader.cpp, tcpheader.h
 * В основном потоке приложения в базовом классе, обрабатывающем очередь сообщений:
 * - создаем экземпляр объекта ServerTcp
 * - создаем в классе функции-слоты для обработки нужных сообщений, реализуем нужную функциональность
 * - привязываем слоты к сигналам.
 * ПРИМЕР:
    server = new ServerTcp(portTcp, QHostAddress("192.168.0.105"), &logger);
    QObject::connect(server, SIGNAL(acceptError  (ClientTcp*  )), this, SLOT(slotAcceptError      (ClientTcp*)));
    QObject::connect(server, SIGNAL(newConnection(ClientTcp*  )), this, SLOT(slotSvrNewConnection (ClientTcp*)));
    QObject::connect(server, SIGNAL(dataready    (ClientTcp*  )), this, SLOT(slotSvrDataready     (ClientTcp*)));
    QObject::connect(server, SIGNAL(disconnected (ClientTcp*  )), this, SLOT(slotSvrDisconnected  (ClientTcp*)));
    QObject::connect(server, SIGNAL(roger        (ClientTcp*  )), this, SLOT(slotRoger            (ClientTcp*)));
    server->start();

 */
#include <QTcpServer>
#include <QTextCodec>
#include "logger.h"

class ClientTcp;

class ServerTcp: public QObject
{
    Q_OBJECT
public:
    ServerTcp(quint16 port, QHostAddress bind=QHostAddress::Any, Logger * = nullptr); // конструктор получает порт и, возможно, интерфейс привязки
    ~ServerTcp();

    QList <class ClientTcp*> clients() { return _clients; }
    void sendToAll(char * data, quint16 length, bool rqAck=false);
    void packsendToAll(char * data, quint16 length, bool compress=false);
    void sendToAllAck(int sec = 0);                                     // отправить всем квитанциию

    bool start();
    void stop();
    void reconfig(quint16 port, QHostAddress bind, Logger *logger);
signals:
    void    clientIdent      (ClientTcp *, QString);
    void	newConnection    (ClientTcp *);               // подключение нового клиента
    void    dataready        (ClientTcp *);                // готовы форматные данные; необходимо их скопировать, т.к. они будут разрушены
    void	acceptError      (ClientTcp *);                // ошибка на сокете
    void    disconnected     (ClientTcp *);               // разрыв соединения
    void    roger            (ClientTcp *);               // принята квитанция

private slots:
    void	slotNewConnection();
    void    slotDataready    (ClientTcp *);           // готовы форматные данные; необходимо их скопировать, т.к. они будут разрушены
    void    slotRawdataready (ClientTcp *);           // готовы форматные данные; необходимо их скопировать, т.к. они будут разрушены
    void    slotDisconnected (ClientTcp *);           // разрыв соединения
    void	slotAcceptError  (ClientTcp *);           // ошибка на сокете
    void    slotRoger        (ClientTcp *);           // принята квитанция

private:
    QList <class ClientTcp*> _clients;
    QTcpServer *tcpServer;
    quint16 port;
    QHostAddress bind;
    Logger      * logger;                                   // логгер для протоколирования
    QString     msg;                                        // строка для формирования сообщений

    void log (QString&);
};

#endif // SERVERTCP_H
