#ifndef HYPERION_SWITCHCOMMANDLINEOPTION_H
#define HYPERION_SWITCHCOMMANDLINEOPTION_H

#include <QtCore>
#include "Option.h"

namespace commandline
{

template <class T>
class SwitchOption: public Option
{
protected:
    QMap<QString, T> _switches;
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

    const QMap<QString, T> &getSwitches() const{return _switches;};
    virtual bool validate(QString &switch_) override{return _switches.contains(switch_);}
    bool hasSwitch(const QString &switch_){return _switches.contains(switch_);}
    void setSwitches(const QMap<QString, T> &_switches){this->_switches = _switches;}
    void addSwitch(const QString &switch_, T value=T()){_switches[switch_] = value;}
    void removeSwitch(const QString &switch_){_switches.remove(switch_);}
	T & switchValue(Parser & parser){return _switches[value(parser)];}
};

}

#endif //HYPERION_SWITCHCOMMANDLINEOPTION_H
