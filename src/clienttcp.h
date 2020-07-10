#ifndef CLIENTTCP_H
#define CLIENTTCP_H

#include "tcpheader.h"
//#include "servertcp.h"
#include "logger.h"
#include <QElapsedTimer>
#include <QTimer>
#include <memory>

// класс ClientTcp представляет собой обертку класса QTcpSocket, который участвует в TCP-соединении как на стороне клиента, так и на стороне сервера
// функция isServer() возвращает истину для серверного соединения

class ClientTcp : public QObject
{
    Q_OBJECT

// слоты для подключения к событиям QTcpSocket
public slots:
    void slotReadyRead      ();
    void slotConnected      ();
    void slotDisconnected   ();
    void slotError (QAbstractSocket::SocketError);
    void errorElapsed();

// сигналы, для подключения слотов вызывающего модуля
signals:
    void connected   (ClientTcp *);                         // установлено соединение
    void disconnected(ClientTcp *);                         // разорвано соединение
    void error       (ClientTcp *);                         // ошибка сокета
    void dataready   (ClientTcp *);                         // готовы форматные данные; необходимо их скопировать, т.к. они будут разрушены
    void rawdataready(ClientTcp *);                         // получены необрамленные данные - отдельный сигнал
    void roger       (ClientTcp *);                         // получена квитанция

public:
    ClientTcp(class ServerTcp *, QTcpSocket  *, std::shared_ptr<Logger> logger);  // конструктор, используемый сервером
    ClientTcp(QString& ipport, std::shared_ptr<Logger> logger = nullptr, bool nodecompress=false, QString idtype = "");
    ClientTcp(QString& remoteIp, int remotePort, std::shared_ptr<Logger> logger = nullptr, bool compress=false, QString idtype = "");
    ~ClientTcp() override;

    void start ();                                          // старт работы сокета
    void stop  ();                                          // останов сокета
    void bind (QString& remoteIp);
    //void compressMode(bool s);
    void setlogger(std::shared_ptr<Logger> p) { logger = p; }
    void setid(QString id) { idtype = id; }
    QString getid() { return idtype; }
    bool isConnected() { return sock->state() == QAbstractSocket::ConnectedState; }
    bool isConnecting() { return sock->state() == QAbstractSocket::ConnectingState; }
    QAbstractSocket::SocketError lasterror() { return _lasterror; }
    QTcpSocket * socket() { return sock; }

    QString remoteIP() { return remoteIp; }
    QString name()                                          // имя в формате IP:порт
        { return QString("%1:%2").arg(remoteIp).arg(remotePort); }

    char * rawData()    { return _data; }                    // "сырые данные" - полный пакет с заголовком
    int    rawLength()  { return _length; }                  // длина пакета с заголовком

    // МОЖНО СРАЗУ ПРОВЕРЯТЬ ТИП ЗАГОЛОВКА (обычный/расширенный) и возвращать нужную длину
    char * data()       { return _data   + (isSignaturedExt() ? sizeof(ExtendedTcpHeader) : sizeof(TcpHeader)); }// пользовательские данные
    int    length()     { return _length - (isSignaturedExt() ? sizeof(ExtendedTcpHeader) : sizeof(TcpHeader)); }// длина пользовательских данных

    bool isCompressed();                                    // проверка префикса сжатых данных

    bool isSignatured   () { return ((TcpHeader    *)_data)->signatured();    }
    bool isSignaturedZip() { return ((TcpHeader    *)_data)->signaturedZip(); }
    bool isSignaturedExt() { return ((ExtendedTcpHeader *)_data)->signatured();    }

    int   packsend(void *, int , bool compress=false);     // упаковка и передача
    int   packsend(QByteArray&, bool compress=false);      // упаковка и передача

    int   packsendExt(void *, int , bool compress=false);  // 2019.06.21. упаковка и передача больших данных
    int   packsendExt(QByteArray&, bool compress=false);   // 2019.06.21. упаковка и передача больших данных

    void   send(void *, int );                              // передача
    void   send(QByteArray&);                               // передача
    void   sendAck();                                       // квитанция

    quint32   getrcvd(int i) { return rcvd[i?1:0]; }           // счетчик: 0-пакетов, 1-байтов
    quint32   getsent(int i) { return sent[i?1:0]; }           // счетчик: 0-пакетов, 1-байтов
    void * userPtr(int i){ return _userPtr[i]; }            // получить пользовательский указатель
    void   setUserPtr(int i, void *p) { _userPtr[i] = p; }  // запомнить пользовательский указатель
    bool   isServer() { return server != nullptr; }         // это серверное соединение ?
    bool   isAcked () { return acked; }                     // квитировано ?
    void   clear() { memset(_data, 0, maxSize); }
    void   setTransparent(bool s) { _transparentMode = s; } // вкл/откл режим ретрансляции сжатых данных без распаковки

    void resetStatistics (){ rcvd[0] = rcvd[1] = sent[0] = sent[1] = 0; }   // Сброс статистики полученных и отправленных данных

    void setAutoRecconect(bool autoReconnect);              // Включить или выключить автореконнект
    static void setDebugMode(bool debug);

private:
    ServerTcp   *server;                                    // владелец - сервер
    int timerId = -1;                                       // Для сохранения id таймера
    QString     remoteIp;
    int         remotePort;
    QString     idtype;                                     // идентификатор типа клиента
    QTcpSocket  *sock;                                      // сокет
    QString     bindIP;                                     // IP привязки или пустая строка
    char        *_data;                                     // указатель на данные (выделяем new char[maxSizeExt];)
    bool        run;                                        // старт/стоп
    QString     msg;                                        // строка для формирования сообщений
   std::shared_ptr<Logger> logger;                                    // логгер для протоколирования
//  bool        nodecompress;                               // не распаковывать уже сжатые данные
    bool        _compress;                                  // не распаковывать уже сжатые данные
    bool        _transparentMode;                           // прием "как есть" без распаковки - используется в шлюзе СПД для ретрансляции
    bool        _autoReconnect = false;                     // автореконнект, если по истечению определённого времени нет принятых данных
    QElapsedTimer     lastDataRead;                         // таймер для отслеживания времени последнего приёма данных

    QAbstractSocket::SocketError _lasterror;                // ошибка
    static const int userdatacount = 3;                     // число пользовательских данных
    static bool debugMode;                                  // true - запись в лог всех сообщений, false - все сообщения выводятся только в троку дебагера
    void        *_userPtr[userdatacount];                   // указатель на данные пользователя
    bool        acked;                                      // квитировано!

    // состояние приема пакета
    int         toRead;                                     // требуемый объем данных
    int         _length;                                    // прочитанный объем данных
    quint32        rcvd[2];                                    // получено   (пакеты, байты)
    quint32        sent[2];                                    // отправлено (пакеты, байты)

    void init ();
    void log (QString&);
    void uncompress();                                      // если данные упакованы - распаковать
protected:
    void timerEvent(QTimerEvent *event) override;           // Обрабатывает события таймера
};

#endif // CLIENTTCP_H
