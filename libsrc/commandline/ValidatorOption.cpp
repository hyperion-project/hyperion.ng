#include "commandline/ValidatorOption.h"
#include "commandline/Parser.h"

using namespace commandline;

bool ValidatorOption::validate(Parser & parser, QString & value)
{
	if (parser.isSet(*this) || !defaultValues().empty())
	{
		int pos = 0;
		validator->fixup(value);
		return validator->validate(value, pos) == QValidator::Acceptable;
	}
	return true;
}

const QValidator *ValidatorOption::getValidator() const
{
	return validator;
}

void ValidatorOption::setValidator(const QValidator *validator)
{
	ValidatorOption::validator = validator;
}

