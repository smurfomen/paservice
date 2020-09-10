#include "archivesaver.h"
#include <QDebug>
#include <QDateTime>
#include <QTime>
#include <QProcess>
/*
    QRegularExpression re("^(@arh_)(?<hour>\\d+).(?<ext>\\w+)$");
    auto match = re.match("@arh_3.arh");
    if(match.hasMatch())
    {
        qDebug()<<match.captured("ext");
        qDebug()<<match.captured("hour");
    }
*/

ArchiveZipper::ArchiveZipper(QString src_directory, QString zip_directory, QString filePrefix)
    :source(src_directory),
     zipdir(zip_directory)
{
    this->zipdir.mkpath(zip_directory);
    mask = QRegularExpression(QString("^(%1)(?<hour>\\d+).(?<ext>\\w+)$").arg(filePrefix));
    addFilter("ext","arh");
    update();
    // раз в 5 минут
    startTimer(5*60000);
}

void ArchiveZipper::update()
{
    Option<QFileInfo> srcfile = findPrevHourFile(source.entryInfoList());

    // дважды копировать один и тот же файл не будем
    if(srcfile.isSome())
    {
        QFileInfo file = srcfile.unwrap();

        QDate dt = QDateTime::currentDateTime().date();
        // если переход на новые сутки, то нужно сохранить последний файл в старый архив;
        if(QTime::currentTime().hour() == 0)
            dt.setDate(dt.year(), dt.day() == 0 ? dt.month()-1 : dt.month(), dt.day()-1);

        QString zipname = dt.toString("yyyy_MM_dd");

        zip(QStringList()<<file.fileName(), zipname);
    }
}


bool ArchiveZipper::filtrate(QString filename)
{
    auto match = mask.match(filename);
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

void ArchiveZipper::zip(const QStringList & files, QString zipname){
    QProcess p;

    zipname = QString("%1/%2.zip").arg(zipdir.absolutePath()).arg(zipname);

    QStringList args;
    args << zipname;

    for(auto & file : files)
        args << file;

    QDir c(QDir::current());
    QDir::setCurrent(source.absolutePath());

    p.start("zip", args);
    p.waitForFinished();
    QDir::setCurrent(c.absolutePath());
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

Option<QFileInfo> ArchiveZipper::findPrevHourFile(QList<QFileInfo> files)
{
    // искомый час - предыдущий - актуальный не трогаем
    quint8 prevHour = QTime::currentTime().hour();
    prevHour = prevHour == 0? 23 : prevHour-1;

    for(auto & file : files)
    {
        if(file.isFile() && filtrate(file.fileName()))
        {
            // проверяем что файл совпадает c предыдущим часом
            auto match = mask.match(file.fileName());
            QString h = match.captured("hour");
            if(!h.isEmpty() && h.toUInt() == prevHour /*&& file.lastModified().toTime_t() >= QDateTime::currentDateTime().toTime_t()-3600*/ )
                return Option<QFileInfo>::Some(file);
        }
    }
    return Option<QFileInfo>::None();
}
