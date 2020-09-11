#include <QCoreApplication>
#include <QHostAddress>
#include "timelistener.h"
#include "ini.h"

#include <singleapplication.h>
#include "archivesaver.h"
/* допустимая входная строка настроек:
 * -multicast [IP:PORT] - для Multicast-соединения
 * -tcp [IP:PORT]       - для TCP-соединения
 */
int main(int argc, char *argv[])
{
    SingleApplication a(argc, argv, true);

    // работаем если единстенный
    if(a.isSecondary()) {
        throw std::logic_error("Приложение уже запущено");
    }

    // если переданы аргументы - используем их
    // порядок аргументов:
    // 1. [-multicast, -tcp] - тип соединения
    // 2. [IP:PORT] - строка соединения
    // 3. /home/pcuser/dcbin/logdir/log.log - путь к логам

    QString inipath = "paservice.ini";
    if(argc > 1)
        inipath = argv[1];

    ini::init(inipath);

    ArchiveZipper zipper(ini::sourceDir, ini::zipDir, ini::sourceFilePrefix);

    for(auto tl : ini::listeners)
        tl->startSinchronize();

    return a.exec();
}
