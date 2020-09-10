#ifndef ARCHIVESAVER_H
#define ARCHIVESAVER_H

#include <QDir>
#include <QRegularExpression>
#include "option.h"

class ArchiveZipper : public QObject
{
    Q_OBJECT

public:
    ArchiveZipper(QString src_directory, QString zip_directory, QString filePrefix);


    void update();
    void zip(const QStringList & tozip, QString zipname);

    bool filtrate(QString filename);

    void timerEvent(QTimerEvent *event) override;

    void addFilter(const QString & name, const QString & filter);
    void dropFilter(const QString & name);
    Option<QFileInfo> findPrevHourFile(QList<QFileInfo> files);

    QMap<QString, QString> filters;
    QDir source;
    QDir zipdir;
    QRegularExpression mask;
};

#endif // ARCHIVESAVER_H
