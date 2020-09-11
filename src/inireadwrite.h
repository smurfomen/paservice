#ifndef INIREADWRITE_H
#define INIREADWRITE_H

#include "logger.h"
#include <QObject>
#include <QHash>
#include <QTextCodec>
#include <QTextStream>
#include <QRegularExpression>


// ВОЗМОЖНА ПРОБЛЕМА С КОДЕКАМИ

class IniReadWrite : public QObject
{
    Q_OBJECT
public:
    IniReadWrite(QString inifile, QObject *parent = nullptr);
    IniReadWrite(QString inifile, Logger *logger, QObject *parent = nullptr);
    ~IniReadWrite();

    bool GetText(QString option, QString& value, QString section = nullptr, int index = 0);// получить значение i-ой опции option в section
    bool GetInt(QString option, int    & value, QString section = nullptr, int index = 0);// получить целое значение i-ой опции option в section
    bool GetBool(QString option, bool    & value, QString section = nullptr, int index = 0);// получить догическое значение i-ой опции option в section
    bool SetField(QString option, QString value, QString section = nullptr, bool flagCreateNewOption = false);// меняет значение опции в указанной секции, flagCreateNewOption = true, создаст опцию, если её нет
    // Если section не задан, то значение опции будет получено из hashUnnamedSection или записано в начало ini-файла в случае SetField


    QString getIniFile();

    bool CreateOption(QString section, QString option, QString value);

    bool reReadIniFile();
    QHash<QString, QStringList>* getHashUnnamedSection () { return hashUnnamedSection; }
    QHash<QString, QHash<QString,QStringList> *>* getHashOfSections(){ return hashOfSections; }

signals:

public slots:

private:
    bool readIniFile (QString& spath, bool bUnicode=true);  // чтение опций из файла
    QFile file;
    QString inifile;
    Logger* logger;
    QHash<QString, QStringList>* hashUnnamedSection;
    QHash<QString, QHash<QString,QStringList> *>* hashOfSections;      // опции
};

#endif // INIREADWRITE_H
