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

	// check if we can create the colors by hex RRGGBB getColors
	QRegularExpression re("(([A-F0-9]){6})(?=(?:..)*)");
	QRegularExpressionMatchIterator i = re.globalMatch(value);

	while (i.hasNext()) {
		QRegularExpressionMatch match = i.next();
		QString captured = match.captured(1);
		_colors.push_back(QColor(QString("#%1").arg(captured)));
	}

	if (!_colors.isEmpty() && (_colors.size() * 6) == value.length())
	{
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
