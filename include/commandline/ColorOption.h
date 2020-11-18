#ifndef HYPERION_COLOROPTION_H
#define HYPERION_COLOROPTION_H

#include "Option.h"
#include <QColor>
#include <QCommandLineParser>

namespace commandline
{

class ColorOption: public Option
{
protected:
    QColor _color;

public:
    ColorOption(const QString &name,
                const QString &description = QString(),
                const QString &valueName = QString(),
                const QString &defaultValue = QString()
    )
        : Option(name, description, valueName, defaultValue)
    {}
    ColorOption(const QStringList &names,
                const QString &description = QString(),
                const QString &valueName = QString(),
                const QString &defaultValue = QString()
    )
        : Option(names, description, valueName, defaultValue)
    {}
    ColorOption(const QCommandLineOption &other)
        : Option(other)
    {}

    bool validate(Parser & parser, QString & value) override;
    QColor getColor(Parser &parser) const { return _color; }
};

}

#endif //HYPERION_COLOROPTION_H
