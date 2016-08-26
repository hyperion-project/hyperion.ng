#include <QRegularExpression>
#include "commandline/ColorOption.h"

using namespace commandline;

bool ColorOption::validate(Parser & parser, QString & value)
{
    // Check if we can create the color by name
    _color = QColor(value);
    if (_color.isValid()) {
        return true;
    }

    // check if we can create the color by hex RRGGBB getColors
    _color = QColor(QString("#%1").arg(value));
    if (_color.isValid()) {
        return true;
    }

    QStringList error;
    error << "Invalid color. A color is specified by a six lettered RRGGBB hex getColors or one of the following names:";
    Q_FOREACH(const QString name, QColor::colorNames()){
        error << "  " << name;
    }
    _error = error.join('\n');

    return false;
}
