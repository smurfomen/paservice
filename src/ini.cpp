#include "ini.h"
#include "inireadwrite.h"

std::shared_ptr<Logger> logger;                  // глобальный класс логгера
QString LOG_DIR = "log/paservice.log";
namespace ini {

QString zipDir;
QString sourceDir;
QString sourceFilePrefix = "@ras_";
QVector<std::shared_ptr<TimeListener>> listeners;

void init(QString inipath)
{
    QFile f(inipath);
    if(f.exists())
    {
        IniReadWrite rdr(inipath);
        QString tmp;

        // логирование
        if(rdr.GetText("LOGDIR", tmp))
            LOG_DIR = tmp;

        Logger::SetLoger(std::shared_ptr<Logger>(new Logger(LOG_DIR, Logger::TimedDaylyFiles)));

        // синхронизация
        if(rdr.GetText("MULTICAST",tmp, "timesync"))
            listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener(tmp)));

        if(rdr.GetText("TCP", tmp, "timesync"))
            listeners.append(std::shared_ptr<TimeListener>(new TcpTimeListener(tmp)));

        // архив
        if(rdr.GetText("ZIP", tmp, "arhive"))
            zipDir = tmp;

        if(rdr.GetText("SOURCE", tmp, "arhive"))
            sourceDir = tmp;

        if(rdr.GetText("PREFIX", tmp, "arhive"))
            sourceFilePrefix = tmp;

    }

    if(listeners.isEmpty())
        listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener("224.1.1.2:64465")));
}

}


