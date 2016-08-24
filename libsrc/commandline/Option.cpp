#include "commandline/Option.h"

using namespace commandline;

bool Option::validate(QString &value)
{
    /* Set to null string if value is not given */
    _value = value.size() ? value : QString();
    _validated = true;
    return true;
}


