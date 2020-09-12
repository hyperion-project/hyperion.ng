#ifndef HYPERION_IMAGEOPTION_H
#define HYPERION_IMAGEOPTION_H

#include "Option.h"
#include <QImage>
#include <QCommandLineParser>

namespace commandline
{

class ImageOption: public Option
{
protected:
    QImage _image;

public:
    ImageOption(const QString &name,
                const QString &description = QString(),
                const QString &valueName = QString(),
                const QString &defaultValue = QString()
    )
        : Option(name, description, valueName, defaultValue)
    {}

    ImageOption(const QStringList &names,
                const QString &description = QString(),
                const QString &valueName = QString(),
                const QString &defaultValue = QString()
    )
        : Option(names, description, valueName, defaultValue)
    {}

    ImageOption(const QCommandLineOption &other)
        : Option(other)
    {}

    bool validate(Parser & parser, QString & value) override;
    QImage& getImage(Parser &parser) { return _image; }
};

}

#endif //HYPERION_IMAGEOPTION_H
