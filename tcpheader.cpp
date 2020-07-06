#include <QRegularExpression>
#include <QRegularExpressionMatch>
#include <QHostAddress>
#include <QHostInfo>
#include <QNetworkInterface>
#include "tcpheader.h"

TcpHeader::TcpHeader()
{
    sign = SIGNATURE;
    len = 4;
    sign = qToLittleEndian<quint16>(sign);
    len = qToLittleEndian<quint16>(len);
}

ExtendedTcpHeader::ExtendedTcpHeader()
{
    sign1 = SIGNATURE;
    sign1 = qToLittleEndian<quint16>(sign1);
    sign2 = 0xffff;
    len = 8;
    len = qToLittleEndian<quint32>(len);
}

// функции qCompress/qUncompress совместимы с используемым в С++/C# проектах форматом ZLIB с особенностями:
// - сжатые данные включают 6-байтный префикс:
// - 4 байта оригинальной длины (похоже, не используются, можно проставить нули)
// - 2 байта сигнатуры ZIP 0x78,0x9C, совпадающие с сигнатурой библиотеки ZLIB
SignaturedPack::SignaturedPack(char * src, int len, bool compress)
{
    pack(src, len, compress);
}

SignaturedPack::SignaturedPack(QByteArray& array, bool compress)
{
    pack(array.data(), array.length(), compress);
}

void SignaturedPack::pack(char * src, int srclength, bool compress)
{
    success = true;
    // 2019.06.21. Проверка на допустимые значения. Если переполнение - либо усекаем данные, либо не передаем вовсе
    if (srclength > (int)sizeof (data))
    {
        qDebug() <<  QObject::tr("Запрос на передачу слишком большого объема данных (%1 бт). Данные усечены до %2").arg(QString::number(srclength), QString::number(sizeof(data)));
        srclength = sizeof (data);
        success = false;
    }
    memset(data, 0, sizeof(data));
    if (compress)
    {
        QByteArray srcarr(srclength, 0);
        for (int i=0; i<srclength; i++)
            srcarr[i] = src[i];
        QByteArray zipped = qCompress(srcarr);              // можно менять степень сжатия!
        if (zipped.length() < srclength - 4)
            memmove (data, zipped.data() + 4, srclength = zipped.length()-4);
        else
        {
            compress = false;
            memcpy(data, src, srclength);
        }

    }
    else
        memcpy(data, src, srclength);
    sign = qToLittleEndian<quint16>(SIGNATURE);
    len = qToLittleEndian<quint16>((quint16)(sizeof(TcpHeader) + srclength));
}


ExtendedSignaturedPack::ExtendedSignaturedPack(char * src, int len, bool compress)
{
    if (compress)
        sign1 = qToLittleEndian<quint16>(SIGNATUREZIP);
    pack(src, len, compress);
}

ExtendedSignaturedPack::ExtendedSignaturedPack(QByteArray& array, bool compress)
{
    if (compress)
        sign1 = qToLittleEndian<quint16>(SIGNATUREZIP);
    pack(array.data(), array.length(), compress);
}

void ExtendedSignaturedPack::pack(char * src, int srclength, bool compress)
{
    success = true;
    // 2019.06.21. Проверка на допустимые значения. Если переполнение - либо усекаем данные, либо не передаем вовсе
    if (srclength > (int)sizeof (data))
    {
        success = false;
        qDebug() <<  QObject::tr("Запрос на передачу слишком большого объема данных (%1 бт). Данные усечены до %2").arg(QString::number(srclength), QString::number(sizeof(data)));
        srclength = sizeof (data);
    }
    memset(data, 0, sizeof(data));
    if (compress)
    {
        QByteArray srcarr(srclength, 0);
        for (int i=0; i<srclength; i++)
            srcarr[i] = src[i];
        QByteArray zipped = qCompress(srcarr);              // можно менять степень сжатия!
        if (zipped.length() < srclength - 8)
            memmove (data, zipped.data() + 4, srclength = zipped.length()-4);
        else
        {
            compress = false;
            memcpy(data, src, srclength);
        }
    }
    else
        memcpy(data, src, srclength);

    sign1 = qToLittleEndian<quint16>(SIGNATURE);
    sign2 = qToLittleEndian<quint16>(0xffff);

    len = qToLittleEndian<quint32>(sizeof(ExtendedTcpHeader) + srclength);
}


QString TcpHeader::ErrorInfo (QAbstractSocket::SocketError error)
{
    switch (error)
    {
        case QAbstractSocket::ConnectionRefusedError            : return QObject::tr("Нет соединения с удаленным хостом");
        case QAbstractSocket::RemoteHostClosedError             : return QObject::tr("Удаленный хост разорвал соединение");
        case QAbstractSocket::HostNotFoundError                 : return QObject::tr("Адрес узла не найден");
        case QAbstractSocket::SocketAccessError                 : return QObject::tr("Не достаточно прав для операции на сокете");
        case QAbstractSocket::SocketResourceError               : return QObject::tr("Не достаточно системных ресурсов");
        case QAbstractSocket::SocketTimeoutError                : return QObject::tr("Истекло время для операции с сокетом");
        case QAbstractSocket::DatagramTooLargeError             : return QObject::tr("Размер дейтаграммы превышаеи максимальный");
        case QAbstractSocket::NetworkError                      : return QObject::tr("Ошибка сетевого оборудования");
        case QAbstractSocket::AddressInUseError                 : return QObject::tr("Адрес привязки уже используется в эксклюзивном режиме");
        case QAbstractSocket::SocketAddressNotAvailableError	: return QObject::tr("Адрес привязки не найден на узле");
        case QAbstractSocket::UnsupportedSocketOperationError	: return QObject::tr("Запрашиваемая операция не поддерживается ОС");
        case QAbstractSocket::ProxyAuthenticationRequiredError	: return QObject::tr("Сокет использует прокси, который запрашивает аутентификацию");
        case QAbstractSocket::UnknownSocketError                :
        default                                                 : return QObject::tr("Неопределённая ошибка сокета");
    }
}

bool TcpHeader::ParseIpPort(QString& ipport, QString& ip, int& port)
{
    bool ret = true;
    ip.clear();
    port = 0;
    QRegularExpressionMatch match = QRegularExpression(R"(\b([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3}):[0-9]{1,5}\b)").match(ipport);
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


// глобальная функция: получить IP-адрес локального хоста, по возможности, реального сетевого адаптера
// возвращает первый IP из списка адресов QHostAddress, удовлетворяющий рег.выражению и не равный "127.0.0.1"
// если не нашли - возвращаем "127.0.0.1"
QString GetHostIp()
{
    QList< QHostAddress > addresses = QNetworkInterface::allAddresses();

    foreach ( const QHostAddress & a, addresses )
    {
        if ( a.protocol() == QAbstractSocket::IPv4Protocol && a.toString() != "127.0.0.1")
        {
            QRegularExpressionMatch match = QRegularExpression(R"(\b([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\.([0-9]{1,3})\b)").match(a.toString());
            if (match.hasMatch())
                return a.toString();
        }
    }
    return "127.0.0.1";
}
