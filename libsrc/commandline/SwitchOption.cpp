#include "commandline/SwitchOption.h"
using namespace commandline;


const QStringList &SwitchOption::get_switches() const
{
    return _switches;
}

void SwitchOption::set_switches(const QStringList &_switches)
{
    SwitchOption::_switches = _switches;
    switchesChanged(_switches);
}

bool SwitchOption::has_switch(const QString &switch_, Qt::CaseSensitivity case_sensitive)
{
    return _switches.contains(switch_, case_sensitive);
}

void SwitchOption::add_switch(const QString &switch_)
{
    _switches.append(switch_);
    switchesChanged(_switches);
}

void SwitchOption::remove_switch(const QString &switch_)
{
    if (_switches.removeAll(switch_)) {
        switchesChanged(_switches);
    }
}

bool SwitchOption::validate(QString &value)
{
    if (_changed) { updateRegularExpression(); }
    return ValidatorOption::validate(value);
}

void SwitchOption::updateRegularExpression()
{
    QString pattern;
    pattern.reserve(_switches.size() * 16);
    pattern += "(";
    int counter = 0;
    foreach(const QString &switch_, _switches) {
        pattern += QRegularExpression::escape(switch_);
        if (++counter != _switches.size()) {
            pattern += "|";
        }
    }
    pattern += ")";
    setValidator(QRegularExpressionValidator(QRegularExpression(pattern)));

    _changed = false;
}

void SwitchOption::switchesChanged(const QStringList &_switches)
{
    _changed = true;
}
