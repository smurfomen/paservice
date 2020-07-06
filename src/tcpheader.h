#ifndef TCPHEADER_H
#define TCPHEADER_H

#include <QTcpSocket>
#include <QAbstractSocket>
#include <QtEndian>

#define SIGNATURE    0xAA55                /// сигнатура пакета
#define SIGNATUREZIP 0x55AA                /// сигнатура сжатого пакета

const int maxSize    = 0x010000 - 8 - 1;   /// 64 Кб
const int maxSizeExt = 0x100000 - 8 - 1;   /// 1  Мб

///\brief Стандартный класс заголовка
class TcpHeader
{
public:
    // статические функции
    static QString ErrorInfo (QAbstractSocket::SocketError error);      /// текстовая информация об ошибке
    static bool ParseIpPort(QString& ipport, QString& ip, int& port);   /// разбор строки IP:порт

    TcpHeader();

    ///\brief Возвращает является ли пакет сжатым
    bool signaturedZip() { return qFromLittleEndian<quint16>(sign) == SIGNATUREZIP; }

    ///\brief Возвращает статус форматированных данных (наличие сигнатуры)
    bool signatured   () { return qFromLittleEndian<quint16>(sign) == SIGNATURE || signaturedZip(); }

    ///\brief Возвращает правильно развернутую длину LittleEndian -> Current Endian
    int length() { return qFromLittleEndian<quint16>(len); }

    ///\brief Возвращает является ли пакет рассширенного типа (ExtendedTcpHeader)
    bool extended  () { return length() == 0xffff;    }

    ///\brief Возвращает максимальный ращмер вмещаемых данных для стандартного пакета
    int capcity(){ return maxSize; }

private:
    quint16 sign;               /// 0xAA55
    quint16 len;                /// общая длина пакета (загловок + данные), если 0xFFFF - расширенный пакет, следующие 3 байта - длина
};

///\brief Расширенный класс заголовка с поддержкой пакетов длиной более 65535 байт
class ExtendedTcpHeader
{
public:
    ExtendedTcpHeader();

    ///\brief Возвращает статус форматированных данных (наличие сигнатуры)
    bool signatured() { return (qFromLittleEndian<quint16>(sign1) == SIGNATURE || qFromLittleEndian<quint16>(sign1)== SIGNATUREZIP) && qFromLittleEndian<quint16>(sign2) == 0xffff; }

    ///\brief Возвращает является ли пакет сжатым
    bool signaturedZip() { return qFromLittleEndian<quint16>(sign1) == SIGNATUREZIP && qFromLittleEndian<quint16>(sign2) == 0xffff; }

    ///\brief Возвращает правильно развернутую длину LittleEndian -> Current Endian
    int length() { return qFromLittleEndian<quint32>(len);  }

    ///\brief Возвращает максимальный размер вмещаемых данных для расширенного пакета
    int capcity(){ return maxSizeExt; }

protected:
    quint16 sign1;              /// 0xAA55
    quint16 sign2;              /// 0xFFFF
    quint32 len;                /// общая длина пакета (загловок + данные), фактически длина занимает только 3 байта, потому 4й байт является резервным
};



///\brief Упаковщик стандартного пакета
class SignaturedPack
{
public:
    SignaturedPack(char * data, int len, bool compress=false);
    SignaturedPack(QByteArray& data, bool compress=false);

    ///\brief Упаковывает данные. После можно передать указатель на класс как указатель на готовый к передаче целевой пакет
    void pack(char * data, int len, bool compress=false);

    ///\brief Возвращает максимальный размер вмещаемых данных для стандартного пакета
    int capacity() { return maxSize; }

    ///\brief Возвращает полную длину пакета включая заголовок LittleEndian -> Current Endian
    quint16 length() {return qFromLittleEndian<quint16>(len);}

private:
    quint16 sign;               /// 0xAA55
    quint16 len;                /// общая длина пакета (загловок + данные)
    char    data[maxSize];      /// данные

public:
    bool    success;            /// флаг успешности компоновки пакета
};



///\brief Упаковщик расширенного пакета
class ExtendedSignaturedPack
{
public:
    ExtendedSignaturedPack(char * data, int len, bool compress=false);
    ExtendedSignaturedPack(QByteArray& data, bool compress=false);

    ///\brief Упаковывает данные. После можно передать указатель на класс как указатель на готовый к передаче целевой пакет
    void pack(char * data, int len, bool compress=false);

    ///\brief Возвращает максимальный размер вмещаемых данных для расширенного пакета
    int capacity() { return maxSizeExt; }

    ///\brief Возвращает полную длину пакета включая заголовок LittleEndian -> Current Endian
    quint32 length() {return qFromLittleEndian<quint32>(len);}

private:
    quint16 sign1;              /// 0xAA55
    quint16 sign2;              /// 0xFFFF
    quint32 len;                /// общая длина пакета (загловок + данные)
    char    data[maxSizeExt];   /// данные

public:
    bool    success;            /// флаг успешности компоновки пакета
};

QString GetHostIp();

#endif // TCPHEADER_H
