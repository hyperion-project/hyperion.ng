#include "commandline/IntOption.h"
#include "commandline/Parser.h"

using namespace commandline;

int IntOption::getInt(Parser &parser, bool *ok, int base)
{
    _int = value(parser).toInt(ok, base);
    return _int;
}

int *IntOption::getIntPtr(Parser &parser, bool *ok, int base)
{
    if (parser.isSet(this)) {
        getInt(parser, ok, base);
        return &_int;
    } else {
        return nullptr;
    }
}
