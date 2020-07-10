#include "ini.h"
#include "inireader.h"

std::shared_ptr<Logger> logger;                  // глобальный класс логгера
QString LOG_DIR = "log/timesync/timesync.log";
namespace ini {

void init(QString inipath)
{
    QFile f(inipath);
    if(f.exists())
    {
        IniReader rdr(inipath);
        QString tmp;
        if(rdr.GetText("MULTICAST",tmp))
            listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener(tmp)));

        if(rdr.GetText("TCP", tmp))
            listeners.append(std::shared_ptr<TimeListener>(new TcpTimeListener(tmp)));

        if(rdr.GetText("LOGDIR", tmp))
            LOG_DIR = tmp;
    }

    logger = std::shared_ptr<Logger>(new Logger(LOG_DIR, true, true));
    Logger::SetLoger(logger.get());
    if(listeners.isEmpty())
    {
        listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener("224.1.1.2:64465")));
    }
}

QVector<std::shared_ptr<TimeListener>> listeners;
}


