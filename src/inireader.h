#ifndef INIREADER_H
#define INIREADER_H

#include <QString>
#include <QFile>
#include <QMap>
#include <QDir>
#include <QFileInfo>
#include <QTextStream>
#include <QRegularExpression>
class IniReader
{
public:
    IniReader(QString inifile);
    ~IniReader();

    bool GetText(QString option, QString& value, int indx = 0);// получить значение i-ой опции option
    bool GetInt (QString option, int    & value, int indx = 0);// получить целое значение i-ой опции option
    bool GetBool(QString option, bool   & value, int indx = 0);// получить догическое значение i-ой опции option

private:
    bool readIniFile (QString& spath, bool bUnicode=true);  // чтение опций из файла
    QFile file;
    QString inifile;
    QMultiMap <QString,QString> options;                    // опции, индексированные по ключу-имени опции
};

#endif // INIREADER_H
