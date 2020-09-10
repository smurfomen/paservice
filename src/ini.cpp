#include "ini.h"
#include "inireader.h"

std::shared_ptr<Logger> logger;                  // глобальный класс логгера
QString LOG_DIR = "log/timesync.log";
namespace ini {

QString zipDir;
QString sourceDir;
QString sourceFilePrefix = "@ras_";

void init(QString inipath)
{
    QFile f(inipath);
    if(f.exists())
    {
        IniReader rdr(inipath);
        QString tmp;

        // логирование
        if(rdr.GetText("LOGDIR", tmp))
            LOG_DIR = tmp;

        // синхронизация
        if(rdr.GetText("MULTICAST",tmp))
            listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener(tmp)));

        if(rdr.GetText("TCP", tmp))
            listeners.append(std::shared_ptr<TimeListener>(new TcpTimeListener(tmp)));

        // архив
        if(rdr.GetText("ZIP", tmp))
            zipDir = tmp;

        if(rdr.GetText("SOURCE", tmp))
            sourceDir = tmp;

        if(rdr.GetText("PREFIX", tmp))
            sourceFilePrefix = tmp;

    }

    Logger::SetLoger(std::shared_ptr<Logger>(new Logger(LOG_DIR, Logger::TimedDaylyFiles)));
    if(listeners.isEmpty())
        listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener("224.1.1.2:64465")));
}

QVector<std::shared_ptr<TimeListener>> listeners;
}


