#include <QCoreApplication>
#include <QHostAddress>
#include "timelistener.h"

std::shared_ptr<Logger> logger;                  // глобальный класс логгера
std::shared_ptr<TimeListener> HandleArgs(QStringList args = QStringList()){
    TimeListener::Type typeService = TimeListener::Multicast;
    QString ipport = "224.1.1.2:64465";

    for(QString & str : args)
    {
        str = str.trimmed();
        str = str.toLower();
        str.replace('-', "");
    }

    if(args.size() >= 1)
        typeService = args.at(0) == "multicast" ? TimeListener::Multicast : args.at(0)== "tcp" ? TimeListener::Tcp : TimeListener::undef;
    if(args.size() >= 2)
        ipport = args.at(1);

    TimeListener * sync = TimeListener::fromType(typeService, ipport);
    return std::shared_ptr<TimeListener>(sync);
}

/* допустимая входная строка настроек:
 * -multicast [IP:PORT] - для Multicast-соединения
 * -tcp [IP:PORT]       - для TCP-соединения
 */
int main(int argc, char *argv[])
{
    time_t tm = time(NULL) - 5000;
    stime(&tm);
    QCoreApplication a(argc, argv);
    logger = std::shared_ptr<Logger>(new Logger(argc >= 4 ? argv[3] : "/home/pcuser/dcbin/log_timesync/timesync_log.log" , true, true));
    Logger::SetLoger(logger.get());
    std::shared_ptr<TimeListener> sync = HandleArgs(argc >= 3 ? QStringList()<< argv[1] << argv[2]: QStringList()<<"multicast"<<"224.1.1.2:64465");
    sync->startSinchronize();
    return a.exec();
}
