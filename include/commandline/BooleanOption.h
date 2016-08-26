#ifndef HYPERION_BOOLEANOPTION_H
#define HYPERION_BOOLEANOPTION_H


#include <QtCore>
#include "Option.h"

namespace commandline
{

class BooleanOption: public Option
{
public:
	BooleanOption(const QString &name,
				const QString &description = QString(),
				const QString &valueName = QString(),
				const QString &defaultValue = QString()
	)
		: Option(name, description, QString(), QString())
	{}
	BooleanOption(const QStringList &names,
				const QString &description = QString(),
				const QString &valueName = QString(),
				const QString &defaultValue = QString()
	)
		: Option(names, description, QString(), QString())
	{}
	BooleanOption(const QCommandLineOption &other)
		: Option(other)
	{}
};

}

#endif //HYPERION_BOOLEANOPTION_H
