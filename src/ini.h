#ifndef INI_H
#define INI_H

#include <memory>
#include <QString>
#include "logger.h"
#include "timelistener.h"
namespace ini {

extern QVector<std::shared_ptr<TimeListener>> listeners;
void init(QString inipath);
}

#endif // INI_H
