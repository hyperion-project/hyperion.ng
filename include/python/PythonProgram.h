#pragma once

#include <python/PythonCompat.h>

#include <QByteArray>
#include <QString>

#include <utils/Logger.h>
#include <python/PythonUtils.h>

class Logger;

class PythonProgram
{
public:
	PythonProgram(const QString & name, QSharedPointer<Logger> log);
	~PythonProgram();

	operator PyThreadState* ()
	{
		return _tstate;
	}

	void execute(const QByteArray &python_code);

private:

	QString _name;
	QSharedPointer<Logger> _log;
	PyThreadState* _tstate;
};
