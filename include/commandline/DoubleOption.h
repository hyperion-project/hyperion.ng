#pragma once

#include <QtCore>
#include "ValidatorOption.h"

namespace commandline
{

class DoubleOption: public ValidatorOption
{
public:
	DoubleOption(const QString &name,
		const QString &description = QString(),
		const QString &valueName = QString(),
		const QString &defaultValue = QString(),
		double minimum = -INFINITY, double maximum = INFINITY, int decimals = 1000)
	: ValidatorOption(name, description, valueName, defaultValue)
	{
		setValidator(new QDoubleValidator(minimum, maximum, decimals));
	}

	DoubleOption(const QStringList &names,
		const QString &description = QString(),
		const QString &valueName = QString(),
		const QString &defaultValue = QString(),
		double minimum = -INFINITY, double maximum = INFINITY, int decimals = 1000)
	: ValidatorOption(names, description, valueName, defaultValue)
	{
		_minimum = minimum;
		_maximum = maximum;
		setValidator(new QDoubleValidator(_minimum, _maximum, decimals));
	}

	DoubleOption(const QCommandLineOption &other, double minimum = -INFINITY, double maximum = INFINITY, int decimals = 1000)
	: ValidatorOption(other)
	{
		_minimum = minimum;
		_maximum = maximum;
		setValidator(new QDoubleValidator(_minimum, _maximum, decimals));
	}

	double getDouble(Parser &parser, bool *ok = 0);
	double *getDoublePtr(Parser &parser, bool *ok = 0);

	bool validate(Parser & parser, QString & value) override;

protected:
	double _double;
	int _minimum;
	int _maximum;
};

}
