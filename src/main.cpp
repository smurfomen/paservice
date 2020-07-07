#include <QCoreApplication>
#include <QHostAddress>
#include "timelistener.h"
#include <QSharedMemory>
#include <QSystemSemaphore>
QSystemSemaphore semaphore("<uniq id>", 1);     // семафор для контроля только 1 экз. ПО
QSharedMemory sharedMemory("<uniq id 2>");      // разделяемая память для для контроля только 1 экз. ПО
namespace glb {
// проверка на то, что нет запущенных экземпляров приложения
bool isUniqueInstanceApp()
{
    semaphore.acquire(); // Поднимаем семафор, запрещая другим экземплярам работать с разделяемой памятью

#ifndef Q_OS_WIN
    // в linux ОС не освобождает разделяемую память при аварийном останове, потому этот объект нужен чтобы очистить
    // необходимо избавиться от мусора c прошлой сессии если был аварийный останов в linux
    if(sharedMemory.attach()){
        sharedMemory.detach();
    }
#endif

    bool unique = true;
    if (sharedMemory.attach())      // пытаемся присоединить экземпляр разделяемой памяти к уже существующему сегменту
        unique = false;
    else
        sharedMemory.create(1);     // В противном случае выделяем 1 байт памяти
    semaphore.release();            // Опускаем семафор

    return unique;
}
}

std::shared_ptr<Logger> logger;                  // глобальный класс логгера
QString TYPE = "multicast";
QString CONNECTION = "224.1.1.2:64465";
#ifdef DEBUG
QString LOG_DIR = "log_timesync/timesync_log.log";
#else
QString LOG_DIR = "/home/pcuser/dcbin/log_timesync/timesync_log.log";
#endif

/* допустимая входная строка настроек:
 * -multicast [IP:PORT] - для Multicast-соединения
 * -tcp [IP:PORT]       - для TCP-соединения
 */
int main(int argc, char *argv[])
{
#ifdef DEBUG
    time_t tm = time(NULL) - 5000;
    stime(&tm);
#endif
    QCoreApplication a(argc, argv);
    // если переданы аргументы - используем их
    // порядок аргументов:
    // 1. [-multicast, -tcp] - тип соединения
    // 2. [IP:PORT] - строка соединения
    // 3. /home/pcuser/dcbin/logdir/log.log - путь к логам

    if(argc >= 3)
    {
        TYPE = argv[1];
        CONNECTION = argv[2];
    }

    if(argc >= 4)
        LOG_DIR = argv[3];

    logger = std::shared_ptr<Logger>(new Logger(LOG_DIR , true, true));

    // проверка на уникальность экземпляра приложения
    if(!glb::isUniqueInstanceApp())
    {
        Logger::LogStr("Совершена попытка запуска второго экземпляра приложения. В создании отказано.");
        return 1;
    }


    Logger::SetLoger(logger.get());

    std::shared_ptr<TimeListener> sync = TimeListener::fromType(argv[1] , argv[2]);
    sync->startSinchronize();
    return a.exec();
}
