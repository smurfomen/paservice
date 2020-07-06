#include <QCoreApplication>
#include <QHostAddress>
#include "timelistener.h"
int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);
    QString ipport = "224.1.1.2:64465";
    TimeListener::Type typeService = TimeListener::Multicast;
    if(argc >= 2)
    {
        QString type = QString(argv[0]).trimmed().replace('-', "").toLower();
        typeService = type == "multicast" ? TimeListener::Multicast : type == "tcp" ? TimeListener::Tcp : TimeListener::undef;
        ipport = QString(argv[1]).trimmed();
    }

    TimeListener * sync = TimeListener::fromType(typeService, ipport);
    sync->startSinchronize();

    return a.exec();
}
