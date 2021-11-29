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

bool DoubleOption::validate(Parser & parser, QString & value)
{
	if (ValidatorOption::validate(parser,value))
	{
		return true;
	}

	_error = QString("Value must be between %1 and %2.").arg(_minimum).arg(_maximum);

	return false;
}
