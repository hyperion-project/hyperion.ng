#include "commandline/DoubleOption.h"
#include "commandline/Parser.h"

using namespace commandline;

double DoubleOption::getDouble(Parser &parser, bool *ok)
{
	_double = value(parser).toDouble(ok);
	return _double;
}

double *DoubleOption::getDoublePtr(Parser &parser, bool *ok)
{
	if (parser.isSet(this))
	{
		getDouble(parser, ok);
		return &_double;
	}

	return nullptr;
}
