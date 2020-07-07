#include "inireader.h"
#include <QRegularExpression>
#include <QTextStream>
#include <QFileInfo>
#include <QDir>
IniReader::IniReader(QString inifile)
{
    this->inifile = inifile;
    readIniFile (this->inifile, true);            // чтение опций из файла
}

IniReader::~IniReader()
{
    //qDebug() << "~IniReader";
}


// чтение опций из файла
// ПРОБЛЕМА: порядок сортировки одноименных опций обратный: последняя опция будет первой
bool IniReader::readIniFile (QString& _spath, bool bUnicode)
{
    //QString root = QDir::currentPath();
    bool ret = true;

    QString spath = _spath;
    QFileInfo fi(_spath);

    if (fi.isRelative())
        spath = QString("%1/%2").arg(QDir::current().absolutePath()).arg(_spath);   // формируем полное имя файла

    QFile file (spath);
    if (file.open(QFile::ReadOnly))
    {
        QTextStream in (&file);
        in.setCodec(bUnicode ? "UTF-8" : "Windows-1251");
        QString str;

        while (!(str = in.readLine().trimmed()).isNull())   // читаем построчно до упора
        {
            if (str[0]==';' || str[0]=='/')                 // отсекаем комментарии
                continue;
            if (QRegularExpression("\\A[^;/=]*=").match(str).hasMatch())
            {
                QString name  = QRegularExpression("[^=^ ]+").match(str).captured();
                QString value = QRegularExpression("(?<=[^;^/]=).+").match(str).captured();
                options.insert(name, value);
            }
        }

    }

    return ret;
}


// ВАЖНО: выходное значение не меняется в случае отсутствия опции
// получить текстовое значение i-ой опции option
bool IniReader::GetText(QString option, QString& value, int indx)
{
    value = "";
    QMultiMap<QString,QString> ::iterator i = options.find(option);
    int j = 0;
    //while (i != options.end() && i.key() == option)
    for (; i != options.end() && i.key() == option; i++)
    {
        if ( j++ == indx)
        {
            value = i.value();
            return true;
        }
    }
    return false;
}

// получить целое значение i-ой опции option
bool IniReader::GetInt (QString option, int& value, int indx)
{
    bool ret = false;
    QString s;
    if (GetText(option, s, indx))
        value = s.toInt(&ret);
    return ret;
}

// получить догическое значение i-ой опции option
bool IniReader::GetBool(QString option, bool   & value, int indx)
{
    bool ret = false;
    QString s;
    if (GetText(option, s, indx))
    {
        s = s.toUpper();
        value = s=="ON" || s=="TRUE";
        ret = true;
    }
    return ret;
}
