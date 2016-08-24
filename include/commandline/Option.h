#ifndef HYPERION_OPTION_H
#define HYPERION_OPTION_H

#include <QCommandLineOption>
#include <QCommandLineParser>

namespace commandline
{

class Parser;

/* Note, this class and all it's derivatives store the validated results for caching. This means that unlike the
 * regular QCommandLineOption it is _not_ idempotent! */
class Option: public QCommandLineOption
{
protected:
    QString _error;
    QString _value;
    bool _validated = false;
public:
    Option(const QString &name,
           const QString &description = QString(),
           const QString &valueName = QString::null,
           const QString &defaultValue = QString()
    )
        : QCommandLineOption(name, description, valueName, defaultValue)
    {}
    Option(const QStringList &names,
           const QString &description = QString(),
           const QString &valueName = QString::null,
           const QString &defaultValue = QString()
    )
        : QCommandLineOption(names, description, valueName, defaultValue)
    {}
    Option(const QCommandLineOption &other)
        : QCommandLineOption(other)
    {}

    virtual bool validate(QString &value);
    bool isValidated()
    { return _validated; }
    QString name()
    { return this->names().last();}
    QString getError()
    { return this->_error; }
    QString value(Parser &parser)
    { return _value; }
    std::string getStdString(Parser &parser)
    { return _value.toStdString(); }
    std::wstring getStdWString(Parser &parser)
    { return _value.toStdWString(); }
};

}

#endif //HYPERION_OPTION_H
