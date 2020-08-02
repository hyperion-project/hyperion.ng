#pragma once

#include <QCommandLineOption>
#include <QCommandLineParser>

namespace commandline
{

class Parser;

/* Note, this class and all it's derivatives store the validated results for caching. This means that unlike the
 * regular QCommandLineOption it is _not_ idempotent! */
class Option: public QCommandLineOption
{
public:
	Option(const QString &name,
		  const QString &description = QString(),
		  const QString &valueName = QString(),
		  const QString &defaultValue = QString()
	);

	Option(const QStringList &names,
		   const QString &description = QString(),
		   const QString &valueName = QString(),
		   const QString &defaultValue = QString()
	);

	Option(const QCommandLineOption &other);
	virtual ~Option() = default;

	virtual bool validate(Parser &parser, QString &value);
	QString name();
	QString getError();
	QString value(Parser &parser);
	const char* getCString(Parser &parser);

protected:
	QString _error;
};

}

