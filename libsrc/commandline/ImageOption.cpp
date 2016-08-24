#include "commandline/ImageOption.h"

using namespace commandline;

bool ImageOption::validate(QString &value)
{
    _image = QImage(value);

    if (_image.isNull())
    {
        _error = QString("File %1 could not be opened as image").arg(value);
        return false;
    }

    return true;
}
