#ifndef LOGGER_H
#define LOGGER_H

#include <QDir>
#include <QMutex>
#include <QTextCodec>
#include <QString>
#include <memory>

class Logger
{    
public:
    enum Flags
    {
        NoFlags = 0x0000,
        WriteTime = 0x0001,                               // пишем дату/время каждого сообщения
        DaylyFiles = 0x0010,                              // отдельный файл на каждый день в формате ИМЯ-ДД.*
        Truncate = 0x0100,                              // удалять существующий файл, если он есть
        TimedDaylyFiles = WriteTime | DaylyFiles
    };

    Logger(QString filename, ulong flags = Flags::NoFlags);
    ~Logger();

    QString log (QString str);                                  // вывод в лог строки  с блокировкой
    QString GetActualFile();                                    // получить актуальный файл
    void    ChangeActualFile(QString);                          // изменение файла лога и его размещения

    // статические открытые функции
    static void SetLoger(std::shared_ptr<Logger> p) { logger = p; }            // установить активный статический логгер
    static void SetTraceLogger(std::shared_ptr<Logger> p) {trace_logger = p;}
    static void LogStr (QString str); // протоколирование строки в SQL и логе; если сохраняем в SQL, отображаем в строке сообщений
    static void LogTrace(QString prefix, void * p, int maxlength = 16);
    static void LogTrace(QString s);
    static void LogTrace(QString prefix, QByteArray data);
    static void LogToAll(QString s);
    static QString TraceFile();
    static QString EventFile();

    static QString GetHex(void * data, int maxlength=16);
    static QString GetHex(QByteArray& array, int maxlength=16);
    static QString GetDirByName(QString);

    static  std::shared_ptr<Logger> logger;                                    // статический экземпляр логгера
    static  std::shared_ptr<Logger> trace_logger;
private:
    QDir    dir;                                                // каталог
    QString sFileNameWithoutExt;
    QString sExt;
    QString sFilePath;                                          // полный путь к файлу лога
    bool	bLogTime;                               			// признак архивирования времени
    bool	bDayly;                                             // признак ведения лога в отд.файлы для кадого дня nsme-ДД.ext
    bool    truncate;
    QMutex* locker;                                             // защита


    void init(QString&);
};
#endif // LOGGER_H
