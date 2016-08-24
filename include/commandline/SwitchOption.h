#ifndef HYPERION_SWITCHCOMMANDLINEOPTION_H
#define HYPERION_SWITCHCOMMANDLINEOPTION_H

#include <QtCore>
#include "ValidatorOption.h"

namespace commandline
{

class SwitchOption: public ValidatorOption
{
private:
    QStringList _switches;
    bool _changed = true;
public:
    SwitchOption(const QString &name,
                 const QString &description = QString(),
                 const QString &valueName = QString(),
                 const QString &defaultValue = QString(),
                 const QStringList switches = QStringList())
        : ValidatorOption(name, description, valueName, defaultValue), _switches(switches)
    {}
    SwitchOption(const QStringList &names,
                 const QString &description = QString(),
                 const QString &valueName = QString(),
                 const QString &defaultValue = QString(),
                 const QStringList switches = QStringList())
        : ValidatorOption(names, description, valueName, defaultValue), _switches(switches)
    {}
    SwitchOption(const QCommandLineOption &other, const QStringList switches)
        : ValidatorOption(other), _switches(switches)
    {}

    const QStringList &get_switches() const;
    virtual bool validate(QString &value) override;
    bool has_switch(const QString &switch_, Qt::CaseSensitivity case_sensitive = Qt::CaseInsensitive);

    void set_switches(const QStringList &_switches);
    void add_switch(const QString &switch_);
    void remove_switch(const QString &switch_);
    void updateRegularExpression();

    void switchesChanged(const QStringList &_switches);
};

}

#endif //HYPERION_SWITCHCOMMANDLINEOPTION_H
