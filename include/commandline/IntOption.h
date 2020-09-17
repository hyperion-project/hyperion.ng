#ifndef HYPERION_INTCOMMANDLINEOPTION_H
#define HYPERION_INTCOMMANDLINEOPTION_H

#include <limits>
#include <QtCore>
#include "ValidatorOption.h"

namespace commandline
{

class IntOption: public ValidatorOption
{
protected:
    int _int;

public:
    IntOption(const QString &name,
              const QString &description = QString(),
              const QString &valueName = QString(),
              const QString &defaultValue = QString(),
              int minimum = std::numeric_limits<int>::min(), int maximum = std::numeric_limits<int>::max())
        : ValidatorOption(name, description, valueName, defaultValue)
    {
        setValidator(new QIntValidator(minimum, maximum));
    }

    IntOption(const QStringList &names,
              const QString &description = QString(),
              const QString &valueName = QString(),
              const QString &defaultValue = QString(),
              int minimum = std::numeric_limits<int>::min(), int maximum = std::numeric_limits<int>::max())
        : ValidatorOption(names, description, valueName, defaultValue)
    {
        setValidator(new QIntValidator(minimum, maximum));
    }

    IntOption(const QCommandLineOption &other,
              int minimum = std::numeric_limits<int>::min(), int maximum = std::numeric_limits<int>::max())
        : ValidatorOption(other)
    {
        setValidator(new QIntValidator(minimum, maximum));
    }

    int getInt(Parser &parser, bool *ok = 0, int base = 10);
    int *getIntPtr(Parser &parser, bool *ok = 0, int base = 10);
};

}

#endif //HYPERION_INTCOMMANDLINEOPTION_H
