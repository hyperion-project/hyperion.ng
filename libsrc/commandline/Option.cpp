#include "commandline/Option.h"
#include "commandline/Parser.h"

using namespace commandline;

bool Option::validate(Parser & parser, QString &value)
{
    /* Set to null string if value is not given */
    _value = value.size() ? value : parser.value(*this);
    _validated = true;
    return true;
}

QString Option::value(Parser &parser)
{
    if (_validated) {
        return _value;
    } else {
        return parser.value(*this);
    }
}

