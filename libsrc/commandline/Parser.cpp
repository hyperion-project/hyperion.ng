#include <QtDebug>
#include <QtGui>
#include "commandline/Parser.h"

using namespace commandline;

bool Parser::parse(const QStringList &arguments)
{
    if (!_parser.parse(arguments)) {
        return false;
    }

    Q_FOREACH(Option * option, _options) {
		    if(!_parser.isSet(*option)){
		  	    continue;
			}

			QString value = this->value(*option);
			if (!option->validate(*this, value)) {
				const QString error = option->getError();
				if (error.size()) {
					_errorText = tr("%1 is not a valid option for %2\n%3").arg(value, option->name(), error);
				}
				else {
					_errorText = tr("%1 is not a valid option for %2").arg(value, option->name());
				}
				return false;
			}
        }
    return true;
}

void Parser::process(const QStringList &arguments)
{
    _parser.process(arguments);
    if (!parse(arguments)) {
        qCritical() << "Error: " << _errorText << "\n";
        showHelp(EXIT_FAILURE);
    }
}

void Parser::process(const QCoreApplication &app)
{
    Q_UNUSED(app);
    process(QCoreApplication::arguments());
}

QString Parser::errorText() const
{
    if (_errorText.size()) {
        return _errorText;
    }
    else {
        return _parser.errorText();
    }
}

bool Parser::addOption(Option &option)
{
    return addOption(&option);
}

bool Parser::addOption(Option * const option)
{
    _options[option->name()] = option;
    return _parser.addOption(*option);
}

