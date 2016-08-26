#include "commandline/ValidatorOption.h"

using namespace commandline;

bool ValidatorOption::validate(Parser & parser, QString & value)
{
    _value = value;
    /* Fix the value if possible */
    this->validator->fixup(value);
    int pos=0;
    return this->validator->validate(value, pos) == QValidator::Acceptable;
}

const QValidator *ValidatorOption::getValidator() const
{
    return validator;
}
void ValidatorOption::setValidator(const QValidator *validator)
{
    ValidatorOption::validator = validator;
}

void ValidatorOption::setValidator(const QValidator &validator)
{
    this->validator = &validator;
}

