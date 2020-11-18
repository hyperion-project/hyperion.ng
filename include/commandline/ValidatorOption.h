#ifndef HYPERION_COMMANDLINEOPTION_H
#define HYPERION_COMMANDLINEOPTION_H

#include <QtCore>
#include <QtGui>
#include <commandline/Option.h>

namespace commandline
{

class ValidatorOption: public Option
{
protected:
    const QValidator *validator;
    virtual void setValidator(const QValidator *validator);

public:
    ValidatorOption(const QString &name,
                    const QString &description = QString(),
                    const QString &valueName = QString(),
                    const QString &defaultValue = QString(),
                    const QValidator *validator = nullptr)
        : Option(name, description, valueName, defaultValue), validator(validator)
    {}

    ValidatorOption(const QStringList &names,
                    const QString &description = QString(),
                    const QString &valueName = QString(),
                    const QString &defaultValue = QString(),
                    const QValidator *validator = nullptr)
        : Option(names, description, valueName, defaultValue), validator(validator)
    {}

    ValidatorOption(const QCommandLineOption &other,
                    const QValidator *validator = nullptr)
        : Option(other), validator(validator)
    {}

    virtual const QValidator *getValidator() const;
    virtual bool validate(Parser & parser, QString &value) override;
};

}

#endif //HYPERION_COMMANDLINEOPTION_H
