#include "inireadwrite.h"

IniReadWrite::IniReadWrite(QString inifile, QObject *parent) : QObject(parent)
{
    hashOfSections = new QHash<QString,QHash<QString,QStringList>*>;
    hashUnnamedSection = new QHash<QString, QStringList>;
    logger = nullptr;
    this->inifile = inifile;
    setParent(parent);
    reReadIniFile();
}

IniReadWrite::IniReadWrite(QString inifile, Logger* logger, QObject *parent) : QObject(parent)
{
    hashOfSections = new QHash<QString,QHash<QString,QStringList>*>;
    hashUnnamedSection = new QHash<QString, QStringList>;
    this->logger = logger;
    this->inifile = inifile;
    setParent(parent);
    reReadIniFile();
}

bool IniReadWrite::reReadIniFile()
{
    return readIniFile(this->inifile, true);
}

bool IniReadWrite::readIniFile (QString& _spath, bool bUnicode)
{
    bool ret = true;

    QString spath = _spath;
    QFileInfo fi(_spath);

    if (fi.isRelative())
    {
        spath = QString("%1/%2").arg(QDir::current().absolutePath()).arg(_spath);   // формируем полное имя файла
    }
    inifile = spath;
    bool flagUnnamedSection = true;

    QFile file (inifile);
    if (file.open(QFile::ReadWrite))
    {
        QTextStream in (&file);
        in.setCodec(bUnicode ? "UTF-8" : "Windows-1251");
        QString str;
        QString keyHash = "";
        QRegularExpression exp("^\\[\\w+\\]$");
        QHash<QString,QStringList>* hash = nullptr;
        while (!(str = in.readLine().trimmed()).isNull())   // читаем построчно до упора
        {
            if (str[0]==';' || str[0]=='/')                 // отсекаем комментарии
                continue;
            if (exp.match(str).hasMatch())
            {
                flagUnnamedSection = false;
                if(hashOfSections->contains(str))
                {
                    keyHash = str;
                    hash = hashOfSections->value(keyHash);
                }else
                {
                    keyHash = str;
                    hash = new QHash<QString,QStringList>;
                }
            } else if (QRegularExpression("\\A[^;\\/=]*=").match(str).hasMatch())
            {
                QString name  = QRegularExpression("[^=^ ]+").match(str).captured();
                QString value = QRegularExpression("(?<=[^;^/]=).+").match(str).captured();

                QStringList list;
                if (!flagUnnamedSection)
                {
                    QStringList list = hash->value(name);
                }

                if(value == nullptr)
                {
                    list.append("");
                } else
                {
                    list.append(value);
                }

                if (flagUnnamedSection){
                    hashUnnamedSection->insert(name, list);
                } else
                {
                    hash->insert(name,list);
                    hashOfSections->insert(keyHash, hash);
                }
            }
        }

    } else
    {
        if(logger != nullptr)
            logger->log("Не получилось открыть файл");
    }
    file.close();
    return ret;
}


bool IniReadWrite::SetField(QString option, QString value, QString section, bool flagCreateNewOption)
{
    bool ret = true;

    option = option.toUpper();
    QMap<QString, QString> map;

    QString string;
    QStringList listFile;
    QFile file(inifile);
    if(file.open(QFile::ReadWrite | QFile::Text))
    {
        QTextStream stream(&file);
        QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
        stream.setCodec(codec);
        string = stream.readAll();
        //собираем строку на подстроки
        listFile = string.split("\n");

        if((listFile.size() == 1) && (listFile[0] == ""))
        {
            if(logger != nullptr){
                logger->log("Файл настроек пустой!!!");
            }
            if(!flagCreateNewOption)
            {
                file.close();
                return false;
            }
        }
        int indexOfSection = 0;
        int flagNoOption = true;
        int flagNoSection = true;
        while (indexOfSection != -1)
        {
            int indexOfOption = -1;
            indexOfSection = listFile.indexOf(QRegularExpression("^\\[" + section + "\\]$", QRegularExpression::CaseInsensitiveOption), indexOfSection); // индекс строки нужной секции
            //найдена ли была секция?
            if (indexOfSection != -1){
                flagNoSection = false;
                for (int i = indexOfSection + 1; i < listFile.size(); i++)
                {
                    QString string = listFile[i];
                    if(section != nullptr){
                        if (QRegularExpression("^\\[" + section + "\\]$\\/i").match(string).hasMatch())
                        {
                            continue;
                        }
                    }
                    if (QRegularExpression("^\\[\\w+\\]$").match(string).hasMatch())
                    {
                        if(indexOfSection != -1) indexOfSection = i;
                        break;
                    }
                    if (QRegularExpression("\\A" + option + "=*").match(string).hasMatch())
                    {
                        flagNoOption = false;
                        indexOfOption = i;
                        break;
                    }
                }
            }
            if (indexOfOption == -1)
            {
                if(indexOfSection != -1) indexOfSection++;
                ret = false;
            } else
            {
                listFile[indexOfOption] = option + "=" + value;
                ret = true;
                break;
            }
        }
        if(flagNoOption)
        {
            if(flagCreateNewOption)
            {
                file.close();
                bool ret = CreateOption(section, option, value);
                if(logger != nullptr)
                {
                    logger->log("В секции " + section + " не была найдена опция " + option +" в файле настроек " + file.fileName() + ". Создаём её. ") + ret;
                }
                return true;
            }else
            {
                if(logger != nullptr)
                {
                    logger->log("В секции " + section + " не была найдена опция " + option +" в файле настроек " + file.fileName());
                }
            }
            ret = false;
        }
        if(flagNoSection)
        {
            if(logger != nullptr)
            {
                logger->log("Секция " + section + " не найдена в файле настроек " + file.fileName());
            }
            ret = false;
        }
        if (ret == true)
        {
            string = "";
            //собираем строки обратно в одну строку
            string = listFile.join("\n");
            stream.seek(0);
            stream << string;
            file.resize(file.pos());
        }
    }
    else
    {
        ret = false;
        if(logger != nullptr)
        {
            logger->log("Не удалось прочитать файл " + inifile +" !");
        }
    }
    file.close();
    return ret;
}

QString IniReadWrite::getIniFile()
{
    return this->inifile;
}

bool  IniReadWrite::CreateOption(QString section, QString option, QString value){

    QString string;
    QStringList listFile;
    QFile file(inifile);
    option = option.toUpper();
    if(file.open(QFile::ReadWrite | QFile::Text))
    {
        QTextStream stream(&file);
        QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
        stream.setCodec(codec);
        string = stream.readAll();
        //собираем строку на подстроки
        listFile = string.split("\n");
    } else
    {
        if(logger != nullptr)
        {
            logger->log("Не удалось прочитать файл " + inifile +" !");
        }
    }

    int indexOfSection = 0;
    if (section != nullptr){
        indexOfSection = listFile.indexOf(QRegularExpression("^\\[" + section + "\\]$", QRegularExpression::CaseInsensitiveOption)); // индекс строки нужной секции
    }
    if(indexOfSection == -1)
    {
        listFile.append("[" + section + "]");
        listFile.append(option + "=" + value);
    } else if(section != nullptr)
    {
        listFile.insert(indexOfSection + 1, option + "=" + value);
    } else if(section == nullptr)
    {
        listFile.insert(0, option + "=" + value);
    }
    string = "";
    //собираем строки обратно в одну строку
    string = listFile.join("\n");
    file.seek(0);
    QTextStream out(&file);
    QTextCodec* codec = QTextCodec::codecForName("Windows-1251");
    out.setCodec(codec);
    out << string;
    file.resize(file.pos());
    file.close();
    return true;
}

bool IniReadWrite::GetText(QString option, QString& value, QString section, int index)
{
    option = option.toUpper();
    int ret = false;
    if(section == nullptr)
    {
        if(hashUnnamedSection->contains(option))
        {
            if(index < hashUnnamedSection->value(option).size())
            {
                value = hashUnnamedSection->value(option).at(index);
            } else
            {
                value = hashUnnamedSection->value(option).first();
            }
            ret = true;
        } else
        {
            if(hashOfSections->values().isEmpty()){
                return false;
            }
            if(hashOfSections->values().first()->contains(option))
            {
                if(index < hashOfSections->values().first()->value(option).size())
                {
                    value = hashOfSections->values().first()->value(option).at(index);
                } else
                {
                    value = hashOfSections->values().first()->value(option).first();
                }
                ret = true;
            }
        }
        return ret;
    }
    section.prepend("[");
    section.append("]");
    if (hashOfSections->contains(section))
    {
        if(hashOfSections->value(section)->contains(option))
        {
            if(index < hashOfSections->value(section)->value(option).size())
            {
                value = hashOfSections->value(section)->value(option).at(index);
            } else
            {
                value = hashOfSections->value(section)->value(option).first();
            }
            ret = true;
        }
    }
    return ret;
}

// получить целое значение i-ой опции option
bool IniReadWrite::GetInt (QString option, int & value, QString section, int index)
{
    bool ret = false;
    QString s;
    if (GetText(option, s, section, index))
    {
        value = s.toInt(&ret);
    }
    return ret;
}

// получить догическое значение i-ой опции option
bool IniReadWrite::GetBool(QString option, bool & value, QString section, int index)
{
    bool ret = false;
    QString s;
    if (GetText(option, s, section, index))
    {
        s = s.toUpper();
        value = s=="ON" || s=="TRUE";
        ret = true;
    }
    return ret;
}

IniReadWrite::~IniReadWrite()
{
    foreach(QString s, hashOfSections->keys())
    {
        delete hashOfSections->value(s);
    }
}


