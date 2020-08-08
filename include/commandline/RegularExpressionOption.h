#ifndef HYPERION_REGULAREXPRESSIONCOMMANDLINEOPTION_H
#define HYPERION_REGULAREXPRESSIONCOMMANDLINEOPTION_H

#include <QtCore>
#include <QRegularExpression>
#include "ValidatorOption.h"

namespace commandline
{

class RegularExpressionOption: public ValidatorOption
{
public:
    RegularExpressionOption(const QString &name,
                            const QString &description = QString(),
                            const QString &valueName = QString(),
                            const QString &defaultValue = QString())
        : ValidatorOption(name, description, valueName, defaultValue)
    {}

    RegularExpressionOption(const QStringList &names,
                            const QString &description = QString(),
                            const QString &valueName = QString(),
                            const QString &defaultValue = QString())
        : ValidatorOption(names, description, valueName, defaultValue)
    {}

    RegularExpressionOption(const QCommandLineOption &other)
        : ValidatorOption(other)
    {}

    RegularExpressionOption(const QString &name,
                            const QString &description = QString(),
                            const QString &valueName = QString(),
                            const QString &defaultValue = QString(),
                            const QRegularExpression &expression = QRegularExpression())
        : ValidatorOption(name, description, valueName, defaultValue)
    {
        setValidator(new QRegularExpressionValidator(expression));
    }

    RegularExpressionOption(const QStringList &names,
                            const QString &description = QString(),
                            const QString &valueName = QString(),
                            const QString &defaultValue = QString(),
                            const QRegularExpression &expression = QRegularExpression())
        : ValidatorOption(names, description, valueName, defaultValue)
    {
        setValidator(new QRegularExpressionValidator(expression));
    }

    RegularExpressionOption(const QCommandLineOption &other,
                            const QRegularExpression &expression = QRegularExpression())
        : ValidatorOption(other)
    {
        setValidator(new QRegularExpressionValidator(expression));
    }

    RegularExpressionOption(const QString &name,
                            const QString &description = QString(),
                            const QString &valueName = QString(),
                            const QString &defaultValue = QString(),
                            const QString &expression = QString())
        : ValidatorOption(name, description, valueName, defaultValue)
    {
        setValidator(new QRegularExpressionValidator(QRegularExpression(expression)));
    }

    RegularExpressionOption(const QStringList &names,
                            const QString &description = QString(),
                            const QString &valueName = QString(),
                            const QString &defaultValue = QString(),
                            const QString &expression = QString())
        : ValidatorOption(names, description, valueName, defaultValue)
    {
        setValidator(new QRegularExpressionValidator(QRegularExpression(expression)));
    }

    RegularExpressionOption(const QCommandLineOption &other,
                            const QString &expression = QString())
        : ValidatorOption(other)
    {
        setValidator(new QRegularExpressionValidator(QRegularExpression(expression)));
    }
};

}

#endif //HYPERION_REGULAREXPRESSIONCOMMANDLINEOPTION_H
