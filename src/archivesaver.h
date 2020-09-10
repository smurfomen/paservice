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
    ArchiveZipper(QString src_directory, QString zip_directory, QRegularExpression m);

    ///\brief выполняет зип архивирование переданных файлов в архив с переданным именем.
    void zip(const QList<QFileInfo> & tozip, QString zipname);

private:

    ///\brief отдает файлы, которые нужно зипануть
    QList<QFileInfo> toNeedZip();

    ///\brief попытаться зипануть доступные за последние сутки файлы.
    ///\details например: в 13.00 зипанет все файлы от 00.00 до 12.00, у которых последнее изменение было в течение последних 24 часов.
    void update();

    bool filtrate(QString filename);

    ///\brief вызывает update
    void timerEvent(QTimerEvent *event) override;

    ///\brief добавить фильтр
    ///\details например regexp == "^(%1)(?<hour>\\d+).(?<ext>\\w+)$"), отфильтровать можно по hour или по ext
    ///\details добавляем фильтр ext на txt и получим только txt файлы
    void addFilter(const QString & name, const QString & filter);

    ///\brief удалить фильтр
    void dropFilter(const QString & name);

    QMap<QString, QString> filters;     // фильтры. Ключ - значение из regexp, которое можно закапчить
    QDir source;                        // директория с исходными архивами
    QDir zipdir;                        // директория с зип архивами
    QRegularExpression regexp;          // регулярное выражение для фильтрации имен файлов
};

#endif // ARCHIVESAVER_H
