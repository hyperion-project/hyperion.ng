#include <QRegularExpression>
#include "commandline/ColorsOption.h"
#include "commandline/Parser.h"

using namespace commandline;

bool ColorsOption::validate(Parser & parser, QString & value)
{
	// Clear any old results
	_colors.clear();

	// Check if we can create the color by name
	QColor color(value);
	if (color.isValid())
	{
		_colors.push_back(color);
		return true;
	}

	// check if we can create the color by hex RRGGBB getColors
	QRegularExpression hexRe("^([0-9A-F]{6})+$", QRegularExpression::CaseInsensitiveOption);
	QRegularExpressionMatch match = hexRe.match(value);
	if(match.hasMatch())
	{
		for(const QString & m : match.capturedTexts())
		{
			_colors.push_back(QColor(QString("#%1").arg(m)));
		}
		return true;
	}

	if(!parser.isSet(*this))
	{
		// Return true if no value is available
		return true;
	}

	_error = QString("Invalid color. A color is specified by a six lettered RRGGBB hex getColors or one of the following names:\n\t- %1").arg(QColor::colorNames().join("\n\t- "));

	return false;
}
