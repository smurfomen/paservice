#ifndef LOGGER_H
#define LOGGER_H

#include <QDir>
#include <QMutex>
#include <QTextCodec>
#include <QString>
class Logger
{
public:
    Logger(QString filename,                                    // имя файла
           bool tmdt        = true,                             // пишем дату/время каждого сообщения
           bool dayly       = false,                            // отдельный файл на каждый день в формате ИМЯ-ДД.*
           bool truncate   = false);                            // удалить существующий файл, если он есть
    ~Logger();

    QString log (QString str);                                  // вывод в лог строки  с блокировкой
    QString GetActualFile();                                    // получить актуальный файл
    void    ChangeActualFile(QString);                          // изменение файла лога и его размещения

    // статические открытые функции
    static void SetLoger(Logger * p) { logger = p; }            // установить активный статический логгер
    static void SetTraceLogger(Logger * p) {trace_logger = p;}
    static void LogStr (QString str); // протоколирование строки в SQL и логе; если сохраняем в SQL, отображаем в строке сообщений
    static void LogTrace(QString prefix, void * p, int maxlength = 16);
    static void LogTrace(QString s);

    static QString GetHex(void * data, int maxlength=16);
    static QString GetHex(QByteArray& array, int maxlength=16);
    static QString GetDirByName(QString);
private:
    static  Logger * logger;                                    // статический экземпляр логгера
    static  Logger * trace_logger;
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
