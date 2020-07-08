#include "ini.h"
#include "inireader.h"

std::shared_ptr<Logger> logger;                  // глобальный класс логгера
QString LOG_DIR = "log_timesync/timesync.log";
namespace ini {
QString logPath(QStringList params)
{
    for(int i = 0; i < params.size(); i++)
    {
        if(params.at(i) == "-logdir" && ++i < params.size())
        {
            LOG_DIR = params.at(i);
            break;
        }
    }
    return LOG_DIR;
}

bool readIni(QString filename){
    QFile f(filename);
    if(f.exists())
    {
        IniReader rdr(filename);
        QString tmp;
        if(rdr.GetText("MULTICAST",tmp))
            listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener(tmp)));
        if(rdr.GetText("TCP", tmp))
            listeners.append(std::shared_ptr<TimeListener>(new TcpTimeListener(tmp)));

        if(rdr.GetText("LOGDIR", tmp))
            logger = std::shared_ptr<Logger>(new Logger(tmp, true, true));
        else
            logger = std::shared_ptr<Logger>(new Logger(LOG_DIR, true, true));
        return listeners.size();
    }
    return false;
}


bool readCmd(QStringList params){
    for(int i = 0; i < params.size(); i++)
    {
        if(i+1 < params.size())
        {
            QString option = params.at(i);
            QString param = params.at(++i);
            if(option == "-multicast")
                listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener(param)));
            else if(option == "-tcp")
                listeners.append(std::shared_ptr<TimeListener>(new TcpTimeListener(param)));
        }
    }

    logger = std::shared_ptr<Logger>(new Logger(ini::logPath(params), true, true));

    return listeners.size();
}


void init(QStringList &params)
{
    QString inipath = "timesync.ini";

    for(int i = 0; i < params.size(); i++)
    {
        if(params.at(i) == "-ini" && ++i < params.size())
        {
            inipath = params.at(i);
            break;
        }
    }

    if(!ini::readIni(inipath))
    {
        if(!ini::readCmd(params))
        {
            listeners.append(std::shared_ptr<TimeListener>(new MulticastTimeListener("224.1.1.2:64465")));
        }
    }
}

QVector<std::shared_ptr<TimeListener>> listeners;
}


