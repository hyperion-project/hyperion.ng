#ifndef HYPERION_COLORSOPTION_H
#define HYPERION_COLORSOPTION_H

#include "Option.h"
#include <QColor>
#include <QCommandLineParser>

namespace commandline
{

class ColorsOption: public Option
{
protected:
    QList<QColor> _colors;

public:
    ColorsOption(const QString &name,
                 const QString &description = QString(),
                 const QString &valueName = QString(),
                 const QString &defaultValue = QString()
    )
        : Option(name, description, valueName, defaultValue)
    {}

    ColorsOption(const QStringList &names,
                 const QString &description = QString(),
                 const QString &valueName = QString(),
                 const QString &defaultValue = QString()
    )
        : Option(names, description, valueName, defaultValue)
    {}

    ColorsOption(const QCommandLineOption &other)
        : Option(other)
    {}

    virtual bool validate(Parser & parser, QString & value) override;
    QList<QColor> getColors(Parser &parser) const { return _colors; }
};

}

#endif //HYPERION_COLOROPTION_H
