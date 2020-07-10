#include <QTextCodec>
#include "clienttcp.h"
#include <QHostAddress>
bool ClientTcp::debugMode = true;

// конструктор, используемый сервером для создания экземпляра, обслуживающего подключение к серверу
ClientTcp::ClientTcp(ServerTcp *server, QTcpSocket  *sock, std::shared_ptr<Logger> logger)
{
    this->server = server;
    this->sock = sock;
    this->logger = logger;

    // прописываем IP адрес клиента и локальный порт
    remoteIp = sock->peerAddress().toString();
    remotePort = sock->localPort();
    _compress = false;
    _transparentMode = false;

    init();
}

// конструктор клиентской стороны для подключения к серверу, заданному IP-адресом и портом
ClientTcp::ClientTcp(QString& ip, int port, std::shared_ptr<Logger> logger, bool compress, QString idtype)
{
    server = nullptr;
    this->remoteIp = ip;
    this->remotePort = port;
    this->logger = logger;
    _compress = compress;
    _transparentMode = false;

    this->idtype = idtype;

    sock = new QTcpSocket(this);
    log(msg = tr("Конструктор ClientTcp %1:%2").arg(remoteIp).arg(remotePort));

    init();
}

// перегруженный конструктор клиентской стороны для подключения к серверу, заданному лексемой IP:ПОРТ
ClientTcp::ClientTcp(QString& ipport, std::shared_ptr<Logger> logger, bool compress, QString idtype)
{
    server = nullptr;
    TcpHeader::ParseIpPort(ipport, remoteIp, remotePort);
    this->logger = logger;
    _compress = compress;
    _transparentMode = false;

    this->idtype = idtype;

    sock = new QTcpSocket(this);
    log(msg = tr("Конструктор ClientTcp %1:%2").arg(remoteIp).arg(remotePort));

    init();
}

// типовая инициализация для всех конструкторов
void ClientTcp::init()
{
    for (int i = 0; i < userdatacount; i++)
        _userPtr [i] = nullptr;
    run = false;
    _data = new char[maxSizeExt];                           // 2019.06.21. Выделяем буфер сразу под расширенный формат
    clear();

    toRead = sizeof(TcpHeader);                             // 4 байта - чтение заголовка
    _length = 0;
    rcvd[0] = rcvd[1] = sent[0] = sent[1] = 0;

    // привязка своих слотов к сигналам QTcpSocket
    connect(sock, SIGNAL(connected   ()), this, SLOT(slotConnected()));
    connect(sock, SIGNAL(readyRead   ()), this, SLOT(slotReadyRead()));
    connect(sock, SIGNAL(disconnected()), this, SLOT(slotDisconnected()));
    connect(sock, SIGNAL(error(QAbstractSocket::SocketError)),this, SLOT(slotError(QAbstractSocket::SocketError)));
}

ClientTcp::~ClientTcp()
{
    delete[] _data;
    log(msg = tr("Деструктор ~ClientTcp: %1:%2").arg(remoteIp).arg(remotePort));
}

// старт работы класса: установка соединения
void ClientTcp::start()
{
    if (!run)
    {
        run = true;
        if (!isServer() && sock != nullptr){
            if(sock->state() != QAbstractSocket::ConnectingState)
            {
                sock->connectToHost(remoteIp,remotePort);
            }
        }
    }
}

// остановка работы класса: разрыв соединения
void ClientTcp::stop()
{
    run = false;
    if (isConnected() || isConnecting())
        sock->disconnectFromHost();
}

// Включить или выключить автореконнект
void ClientTcp::setAutoRecconect(bool autoReconnect)
{
    this->_autoReconnect = autoReconnect;
    if(autoReconnect)
    {
        if(timerId == -1)
        {
            timerId = startTimer(1000);
        }
    } else
    {
        if(timerId != -1)
        {
            killTimer(timerId);
            timerId = -1;
        }
    }
}

void ClientTcp::setDebugMode(bool debug)
{
    debugMode = debug;
}

// прием данных
// форматные данные:
// - WORD -  сигнатуры: 0xAA55
// - WORD -  длина пакета, включая заголовок
// - данные
//
// toRead - требуемая длина; если toRead==длине заголовка, принимаем заголовок, определяем длину пакета, и toRead = длине пакета
void ClientTcp::slotReadyRead()
{
    if (sock == nullptr)
        return;
    while (sock->isOpen()) // Пока есть что принять или передать
    {
        _length += sock->read(_data+_length, toRead-_length);   // читаем в буфер со смещением length
        if (_length < toRead)
            return;

        lastDataRead.restart();
        if (_length==sizeof(ExtendedTcpHeader))                  // расширенный 8-ми байтный заголовок
            toRead = ((ExtendedTcpHeader *)_data)->length();     // общая длина пакета (загловок + данные)

        else if (_length==sizeof(TcpHeader))                     // стандартный 4-х байтный заголовок
        {
            // анализ первых 4-х байтов
            if (((TcpHeader *)_data)->signatured())
            {
                // приняли заголовок пакета
                toRead = ((TcpHeader *)_data)->length();     // общая длина пакета (загловок + данные)
                if ((quint16)toRead == 0xffff)
                {
                    toRead = sizeof(ExtendedTcpHeader);
                    _length += sock->read(_data +_length, toRead-_length);   // читаем в буфер со смещением length
                }
                else
                {
                    //qDebug() << "Заголовок";
                    if (toRead==4)
                    {
                        rcvd[0]++; rcvd[1] += toRead;           // инкремент
                        //qDebug() << "Квитанция";
                        acked = true;
                        emit roger(this);                       // уведомляем о получении квитанции отпракой сигнала

                        // фикс 2018.06.06. Данные в потоке от рзаных отправок могут накладываться, поэтому
                        // данные надо выбирать полностью! Выходить надо когда данных уже нет
                        toRead = sizeof(TcpHeader);
                        _length = 0;
                        continue;                               //return;
                    }
                }
            }
            else
            {
                // первые 4 - байта - не заголовок, значит, неформатные данные
                // 2018.06.06. Похоже, я не успеваю принять идентификацию клиента - не попадаю сюда
                //             и вообще не вижу приема данных идентификации
                //             читаем все, что есть
                _length += sock->read(_data+_length, maxSizeExt-_length); // 65536
                rcvd[1] += _length;                          // инкремент

                emit rawdataready(this);

                _length = 0;
                toRead = sizeof(TcpHeader);
                continue;                                   // return; данные надо выбирать полностью! Выходить надо когда данных уже нет
            }
        }
        else
        {
            // получили требуемую пачку данных (length >= toRead)
            rcvd[0]++; rcvd[1] += _length;                   // инкремент

            // распаковку делать не всегда, если это ретрансляция - можно не распаковывать, а передавать как есть
            uncompress();

            emit dataready (this);                          // обращение к подключенным слотам; они должны гарантированно забрать данные

            _length = 0;
            toRead = sizeof(TcpHeader);
        }
    }
}

// ДОРАБОТАНО С ПОДДЕРЖКОЙ РАЗНОЙ СТЕПЕНИ СЖАТИЯ: если сжатие не по умолчанию, сигнатура ZIP 0x78,0x9C не используется,
// поэтому вводится отдельный вид сигнатуры сжатого пакета:
//  #define SIGNATURE    0xAA55                                 // сигнатура пакета общая (поддерживаются потоки Windows C++/C#)
//  #define SIGNATUREZIP 0x55AA                                 // сигнатура сжатого пакета (только QT версия)
// если принятые данные упакованы - распаковать и скорректировать поле длины
// ПОКА НЕ УЧИТЫВАЕМ РАСШИРЕННЫЙ ЗАГОЛОВОК
// функции qCompress/qUncompress совместимы с используемым в С++/C# проектах форматом ZLIB с особенностями:
// - сжатые данные включают 6-байтный префикс:
// - 4 байта оригинальной длины (похоже, не используются, можно проставить нули)
// - (ВАЖНО: ТОЛЬКО ПРИ СЖАТИИ ПО УМОЛЧАНИЮ!) 2 байта сигнатуры ZIP 0x78,0x9C, совпадающие с сигнатурой библиотеки ZLIB
//   С учетом того, что в сжатых пакетах длина заголовка пакета составляет 4 байта, за которым идет сигнатура ZIP 0x78,0x9C,
//   заголовок пакета используется как префикс
void ClientTcp::uncompress()
{
    if (!_transparentMode && isCompressed())
    {
        // 2019.06.21. обработка расширенного сжатого пакета и обычного сжатого пакета отличается
        if (isSignaturedExt())                                  // РАСШИРЕННЫЙ
        {
            QByteArray zip(_data + 4, _length);                 // смещение = -4 относительно _data
            zip[0] = 0;                                         // обнуляем служебные поля zip-архива
            zip[1] = 0;                                         // обнуляем служебные поля
            zip[2] = 0;                                         // обнуляем служебные поля
            zip[3] = 0;                                         // обнуляем служебные поля
            QByteArray unzip = qUncompress(zip);
            _length = unzip.length() + 8;                       // поле длины пакета включает длину заголовка, учтем!
            * (int *)&_data[4] = _length;

            for (int i=0; i< unzip.length(); i++)               // копируем данные
                _data[8+i] = unzip[i];
        }
        else
        {                                                       // ОБЫЧНЫЙ
            QByteArray zip(_data, _length);                     // смещение = -4 относительно _data
            quint8 h1 = zip[0];                                   // сохраняю сигнатуру
            quint8 h2 = zip[1];
            zip[0] = 0;                                         // обнуляем служебные поля zip-архива
            zip[1] = 0;                                         // обнуляем служебные поля
            zip[2] = 0;                                         // обнуляем служебные поля
            zip[3] = 0;                                         // обнуляем служебные поля
            QByteArray unzip = qUncompress(zip);
            _length = unzip.length() + 4;                       // поле длины пакета включает длину заголовка, учтем!
            _data[2] = (quint8)_length;                           // УЧЕСТЬ РАСШИРЕННЫЙ ПАКЕТ!
            _data[3] = (quint8)((_length) >> 8);
            for (int i=0; i< unzip.length(); i++)               // копируем данные
                _data[4+i] = unzip[i];

            zip[0] = h1;                                        // восстанавливаем сигнатуру
            zip[1] = h2;                                        //
        }
    }
}

// проверка префикса сжатых данных - работает только при сжатии по умолчанию
bool ClientTcp::isCompressed()
{
    if (isSignaturedZip())
        return true;                                        // явно указанное сжатие
    int lengthheader = isSignaturedExt() ? sizeof(ExtendedTcpHeader) : sizeof(TcpHeader);    // учитываем расширенный заголовок
    return _length > lengthheader + 2 && (quint8)_data[lengthheader] == 0x78 && (quint8)_data[lengthheader +1 ] == 0x9C;
}

// установлено соединение
void ClientTcp::slotConnected()
{
    log(msg = tr("ClientTcp. Установлено соединение c хостом %1:%2").arg(remoteIp).arg(remotePort));

    // если определен тип, отправляет тип удаленному серверу, преобразовав в кодировку Windows-1251
    if (idtype.length())
    {
        QByteArray id = QTextCodec::codecForName("Windows-1251")->fromUnicode(idtype);
        id.append('\0');
        send(id);
    }
    emit connected (this);
}

// разорвано соединение
void ClientTcp::slotDisconnected()
{
    log(msg = tr("ClientTcp. Разрыв соединения c хостом %1").arg(name()));
    emit disconnected (this);
    if (!isServer() && run)
        sock->connectToHost(remoteIp,remotePort);
}

// ошибка
void ClientTcp::slotError(QAbstractSocket::SocketError er)
{
    _lasterror = er;
    log(msg = tr("ClientTcp. Клиент %1. Ошибка: %2").arg(name()).arg(sock->errorString()/*TcpHeader::ErrorInfo(er)*/));
    emit error(this);
    QTimer::singleShot(3000, this,SLOT(errorElapsed()));
}

void ClientTcp::errorElapsed()
{
    if (!isServer() && run && !isConnected() && !isConnecting())
    {
        sock->connectToHost(remoteIp,remotePort);
    }
}

void ClientTcp::log(QString& msg)
{
    if (logger && debugMode)
        logger->log(msg);
    else
        qDebug() << msg;
}

// упаковка и передача
int ClientTcp::packsend(void *data, int length, bool compress)
{
    SignaturedPack pack((char*)data, length, compress);
    if (!pack.success)
    {
        log(msg = tr("Переполнение при передаче. Усечение данных с %1 до %2 байт").arg(length).arg(pack.capacity()));
    }
    send((char*)&pack, pack.length());
    return pack.capacity();
}

// упаковка и передача
int ClientTcp::packsend(QByteArray& array, bool compress)
{
    SignaturedPack pack(array, compress);
    if (!pack.success)
    {
        log(msg = tr("Переполнение при передаче. Усечение данных с %1 до %2 байт").arg(array.length()).arg(pack.capacity()));
    }
    send((char*)&pack, pack.length());
    return pack.capacity();
}

// упаковка и передача расширенного пакета
int ClientTcp::packsendExt(void *data, int length, bool compress)
{
    ExtendedSignaturedPack pack((char*)data, length, compress);
    if (!pack.success)
    {
        log(msg = tr("Переполнение при передаче. Усечение данных с %1 до %2 байт").arg(length).arg(pack.capacity()));
    }
    send ((char*)&pack, pack.length());
    return pack.capacity();
}

// упаковка и передача расширенного пакета
int ClientTcp::packsendExt(QByteArray& array, bool compress)
{
    ExtendedSignaturedPack pack(array, compress);
    if (!pack.success)
    {
        log(msg = tr("Переполнение при передаче. Усечение данных с %1 до %2 байт").arg(array.length()).arg(pack.capacity()));
    }
    send((char*)&pack, pack.length());
    return pack.capacity();
}

// передача подготовленного блока данных "как есть"
void ClientTcp::send(void * p, int length)
{
    if (isConnected())
    {
        sock->write((char*)p,length);
        sent[0]++; sent[1] += length;
        acked = false;
    }
    else
        log(msg = tr("ClientTcp. Игнорируем отправку данных в разорванное соединение %1").arg(name()));
}

// передача массива
void ClientTcp::send(QByteArray& array)
{
    send(array.data(), array.length());
}

void ClientTcp::sendAck()
{
    TcpHeader ack;
    send(&ack, sizeof(ack));
}

void ClientTcp::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    if(lastDataRead.restart() >= 5000)
    {
        stop();
        start();
    }
}
