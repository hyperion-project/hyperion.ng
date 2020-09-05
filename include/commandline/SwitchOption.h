#pragma once

#include <QtCore>
#include "Option.h"

namespace commandline
{

template <class T>
class SwitchOption: public Option
{
public:
	SwitchOption(const QString &name,
	             const QString &description = QString(),
	             const QString &valueName = QString(),
	             const QString &defaultValue = QString(),
	             const QMap<QString, T> &switches=QMap<QString, T>())
		: Option(name, description, valueName, defaultValue), _switches(switches)
	{}

	SwitchOption(const QStringList &names,
	             const QString &description = QString(),
	             const QString &valueName = QString(),
	             const QString &defaultValue = QString(),
	             const QMap<QString, T> &switches=QMap<QString, T>())
		: Option(names, description, valueName, defaultValue), _switches(switches)
	{}

	SwitchOption(const QCommandLineOption &other, const QMap<QString, T> &switches)
		: Option(other), _switches(switches)
	{}

	const QMap<QString, T> &getSwitches() const                      { return _switches; }
	virtual bool validate(Parser &parser, QString &switch_) override { return hasSwitch(switch_); }
	bool hasSwitch(const QString &switch_)                           { return _switches.contains(switch_.toLower()); }
	void setSwitches(const QMap<QString, T> &_switches)              { this->_switches = _switches; }
	void addSwitch(const QString &switch_, T value=T())              { _switches[switch_.toLower()] = value; }
	void removeSwitch(const QString &switch_)                        { _switches.remove(switch_.toLower()); }
	T & switchValue(Parser & parser)                                 { return _switches[value(parser).toLower()]; }

protected:
	QMap<QString, T> _switches;
};

}
