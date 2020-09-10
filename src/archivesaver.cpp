#include "archivesaver.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QProcess>

ArchiveZipper::ArchiveZipper(QString src_directory, QString zip_directory, QString filePrefix)
{
    source.setPath(src_directory);
    zipdir.setPath(zip_directory);
    zipdir.mkpath(zip_directory);
    regexp = QRegularExpression(QString("^(%1)(?<hour>\\d+).(?<ext>\\w+)$").arg(filePrefix));
    addFilter("ext","arh");
    update();

    // раз в 5 минут
    startTimer(5*60000);
}

ArchiveZipper::ArchiveZipper(QString src_directory, QString zip_directory, QRegularExpression m)
{
    source.setPath(src_directory);
    zipdir.setPath(zip_directory);
    zipdir.mkpath(zip_directory);
    regexp = m;
    update();

    // раз в 5 минут
    startTimer(5*60000);
}

QList<QFileInfo> ArchiveZipper::toNeedZip()
{
    // находим предыдущий час - максимальный час архива который можно трогать
    quint8 hour = QTime::currentTime().hour();
    hour = hour == 0? 23 : hour-1;

    QList<QFileInfo> list;

    // найти список файлов, которые доступны для зипа в рамках одного дня
    for(auto & f : source.entryInfoList())
    {
        // если имя файла проходит все фильтры
        if(f.isFile() && filtrate(f.fileName()))
        {
            // проверяем что файл совпадает c предыдущим часом - в названии файла цифра @ras_11.arh - 11й час архива
            auto match = regexp.match(f.fileName());
            QString h = match.captured("hour");
            quint8 fileHour = h.toUInt();
            if(     !h.isEmpty()                                                                        // 1. разматчили
                    && fileHour <= hour                                                                 // 2. час файла меньше или равен максимально допустимому
                    && fileHour != QTime::currentTime().hour()                                          // 3. час файла не равен актуальному
                    && f.lastModified().toTime_t() > QDateTime::currentDateTime().toTime_t() - 24*3600  // 4. последний раз был модифицирован не более суток назад
              )
                list.append(f);
        }
    }

    Q_ASSERT(list.size() < 24);

    return list;
}

void ArchiveZipper::update()
{
    // файлы которые можно зипануть за последние сутки
    auto list = toNeedZip();

    if(list.size())
    {
        QDate dt = QDateTime::currentDateTime().date();

        // если полночь - архивируем с именем уже вчерашней даты
        if(QTime::currentTime().hour() == 0)
            dt.setDate(dt.year(), dt.day() == 0 ? dt.month()-1 : dt.month(), dt.day()-1);

        QString zipname = dt.toString("yyyy_MM_dd");

        zip(list, zipname);
    }
}


bool ArchiveZipper::filtrate(QString filename)
{
    auto match = regexp.match(filename);
    if(!match.hasMatch())
        return false;

    for(auto & key : filters.keys())
    {
        if(match.captured(key) != filters.value(key))
            return false;
    }
    return true;
}


void ArchiveZipper::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    update();
}

void ArchiveZipper::zip(const QList<QFileInfo> & files, QString zipname){
    QProcess p;

    QFileInfo fizip (zipname);

    // формируем имя архива
    if(fizip.suffix() == "")
        zipname = QString("%1").arg(zipname);

    if(!fizip.isAbsolute())
        zipname = QString("%1/%2").arg(zipdir.absolutePath()).arg(zipname);

    QStringList args;
    args << zipname;

    for(auto & file : files)
        args << file.fileName();

    QDir prevCurrentDir(QDir::current());
    QDir::setCurrent(source.absolutePath());

    p.start("zip", args);
    p.waitForFinished();
    QDir::setCurrent(prevCurrentDir.absolutePath());
}

void ArchiveZipper::addFilter(const QString & name, const QString & filter)
{
    filters.insert(name,filter);
}

void ArchiveZipper::dropFilter(const QString &name)
{
    if(filters.contains(name))
        filters.remove(name);
}
