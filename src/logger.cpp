#include <QMutex>
#include <QFileInfo>
#include <QTextStream>
#include <QDir>
#include <QDate>
#include <QDebug>
#include <QTextCodec>

#include "logger.h"

std::shared_ptr<Logger> Logger::logger = nullptr;                                            // статический экземпляр логгера
std::shared_ptr<Logger> Logger::trace_logger = nullptr;

Logger::Logger(QString filename, ulong flags)
{
    bLogTime    = (flags & WriteTime) > 0;
    bDayly      = (flags & DaylyFiles) > 0;
    truncate    = (flags & Truncate) > 0;
    locker    = new QMutex();
    init(filename);
}

Logger::~Logger()
{
    delete locker;
}

// получить каталог по имени файла (прямые слэши)
QString Logger::GetDirByName(QString filename)
{
    QFileInfo fi(filename);
    return fi.isRelative() ?
                QString("%1/%2/").arg(QDir::current().absolutePath()).arg(fi.path()) :
                fi.dir().path();
}


void Logger::init(QString& filename)
{
    sFilePath = filename;
    QFileInfo fi(sFilePath);
    if (fi.isRelative())
    {
        // создаем папку лога
        if(!fi.dir().exists())
            fi.dir().mkpath(fi.absolutePath());
        sFilePath = fi.absoluteFilePath();
    }

    sFileNameWithoutExt = fi.baseName();
    sExt = fi.completeSuffix();
    dir  = fi.dir();

    if (truncate)
        QFile(GetActualFile()).remove();
}

// актуальный файл в случае ежедневного архив формируется добавлением даты в формате: ПУТЬ/ИМЯ-ДД.*
QString Logger::GetActualFile ()
{
    return bDayly ? QString("%1/%2-%3.%4").arg(dir.absolutePath()).arg(sFileNameWithoutExt).arg(QDate::currentDate().toString("dd")).arg(sExt) : sFilePath;
}

// изменение файла лога и его размещения
void Logger::ChangeActualFile(QString filename)
{
    init(filename);
}


// вывод в лог строки с блокировкой
QString Logger::log (QString str)
{
    locker->lock();

    QString filename = GetActualFile();
    QFile file (filename);

    // если архив ежедневный, проверить дату обновления файла, если месячной давности - удалить
    if (bDayly && QFileInfo(file).lastModified().date().month() != QDate::currentDate().month())
    {
        file.remove();
    }

    if (file.open(QFile::Append))
    {
#ifdef Q_OS_WIN
        QTextCodec::setCodecForLocale( QTextCodec::codecForName("Windows-1251"));
#endif
        QTextStream out (&file);
        QString msg = QString("%1%2").arg(bLogTime?QDateTime::currentDateTime().toString("dd.MM.yyyy hh:mm:ss  "):"").arg(str);
        out << msg <<"\r\n";
#ifdef Q_OS_WIN
        QTextCodec::setCodecForLocale( QTextCodec::codecForName("CP866"));
#endif
        qDebug() << msg.toStdString().c_str();                    // дублируем отладочном окне
    }

    locker->unlock();
    return str;
}


// статическая функция протоколирования строки в SQL и логе; если сохраняем в SQL, отображаем в строке сообщений
// странно, но такое решение приводит к искажению кодировки входной строки при вызове функции из другого потока
void Logger::LogStr (QString str)
{
    if (logger != nullptr)
        logger->log(str);
    else
        qDebug() << "ERR WRITE LOG:" <<  qPrintable(str);
}

void Logger::LogTrace(QString prefix, void *p, int maxlength)
{
    QString str = prefix + GetHex(p,maxlength);
    Logger::LogTrace(str);
}

void Logger::LogTrace(QString prefix, QByteArray data)
{
    Logger::LogTrace(prefix, data.data(), data.length());
}


void Logger::LogTrace(QString s)
{
    if (trace_logger != nullptr)
        trace_logger->log(s);
    else
        qDebug() << "ERR WRITE LOG:" << qPrintable(s);
}

void Logger::LogToAll(QString s)
{
    Logger::LogStr(s);
    Logger::LogTrace(s);
}

QString Logger::TraceFile()
{
    return trace_logger != nullptr? trace_logger->GetActualFile()  : "";
}

QString Logger::EventFile()
{
    return logger != nullptr? logger->GetActualFile()  : "";
}


// получить 16-ричную строку
QString Logger::GetHex(void * data, int length)
{
    QString tmp;
    for (int i=0; i<length; i++)
    {
        tmp += QString("%1 ").arg(((uchar*)data)[i],2,16,QChar('0')).toUpper();
    }
    return tmp;
}

QString Logger::GetHex(QByteArray& array, int maxlength)
{
    QString tmp;
    for (int i=0; i<array.length() && i<maxlength; i++)
    {
        tmp += QString("%1 ").arg((uchar)array[i],2,16,QChar('0')).toUpper();
    }
    return tmp;
}
