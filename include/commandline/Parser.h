#pragma once

#include <QtCore>
#include "ColorOption.h"
#include "ColorsOption.h"
#include "DoubleOption.h"
#include "ImageOption.h"
#include "IntOption.h"
#include "Option.h"
#include "RegularExpressionOption.h"
#include "SwitchOption.h"
#include "ValidatorOption.h"
#include "BooleanOption.h"

namespace commandline
{

class Parser : public QObject
{
protected:
	QHash<QString, Option *> _options;
	QString _errorText;
	/* No public inheritance because we need to modify a few methods */
	QCommandLineParser _parser;

	QStringList _getNames(const char shortOption, const QString& longOption);
	QString _getDescription(const QString& description, const QString& default_=QString());

public:
	~Parser() override;

	bool parse(const QStringList &arguments);
	void process(const QStringList &arguments);
	void process(const QCoreApplication &app);
	QString errorText() const;

	template<class OptionT, class ... Args>
	OptionT &add(
		const char shortOption,
		const QString longOption,
		const QString description,
		const QString default_,
		Args ... args)
	{
		OptionT * option = new OptionT(
			_getNames(shortOption, longOption),
			_getDescription(description, default_),
			longOption,
			default_,
			args...);
		addOption(option);
		return *option;
	}

	/* gcc does not support default arguments for variadic templates which
	 * makes this method necessary */
	template<class OptionT>
	OptionT &add(
		const char shortOption,
		const QString longOption,
		const QString description,
		const QString default_ = QString())
	{
		OptionT * option = new OptionT(
			_getNames(shortOption, longOption),
			_getDescription(description, default_),
			longOption,
			default_);
		addOption(option);
		return *option;
	}

	template<class OptionT>
	OptionT &addHidden(
		const char shortOption,
		const QString longOption,
		const QString description,
		const QString default_ = QString())
	{
		OptionT * option = new OptionT(
			_getNames(shortOption, longOption),
			_getDescription(description, default_),
			longOption,
			default_);
		#if (QT_VERSION >= QT_VERSION_CHECK(5, 8, 0))
		option->setFlags(QCommandLineOption::HiddenFromHelp);
		#else
		option->setHidden(true);
		#endif

		addOption(option);
		return *option;
	}

	Parser(const QString& description = QString())
	{
		if(description.size())
			setApplicationDescription(description);
	}

	QCommandLineOption addHelpOption()
	{
		return _parser.addHelpOption();
	}

	bool addOption(Option &option);
	bool addOption(Option *option);
	void addPositionalArgument(const QString &name, const QString &description, const QString &syntax = QString())
	{
		_parser.addPositionalArgument(name, description, syntax);
	}

	QCommandLineOption addVersionOption()
	{
		return _parser.addVersionOption();
	}

	QString applicationDescription() const
	{
		return _parser.applicationDescription();
	}

	void clearPositionalArguments()
	{
		_parser.clearPositionalArguments();
	}

	QString helpText() const
	{
		return _parser.helpText();
	}

	bool isSet(const QString &name) const
	{
		return _parser.isSet(name);
	}

	bool isSet(const Option &option) const
	{
		return _parser.isSet(option);
	}

	bool isSet(const Option *option) const
	{
		return _parser.isSet(*option);
	}

	QStringList optionNames() const
	{
		return _parser.optionNames();
	}

	QStringList positionalArguments() const
	{
		return _parser.positionalArguments();
	}

	void setApplicationDescription(const QString &description)
	{
		_parser.setApplicationDescription(description);
	}

	void setSingleDashWordOptionMode(QCommandLineParser::SingleDashWordOptionMode singleDashWordOptionMode)
	{
		_parser.setSingleDashWordOptionMode(singleDashWordOptionMode);
	}

	[[ noreturn ]] void showHelp(int exitCode = 0)
	{
		_parser.showHelp(exitCode);
	}

	QStringList unknownOptionNames() const
	{
		return _parser.unknownOptionNames();
	}

	QString value(const QString &optionName) const
	{
		return _parser.value(optionName);
	}

	QString value(const Option &option) const
	{
		return _parser.value(option);
	}

	QStringList values(const QString &optionName) const
	{
		return _parser.values(optionName);
	}

	QStringList values(const Option &option) const
	{
		return _parser.values(option);
	}
};

}
