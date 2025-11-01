#pragma once

#include <QByteArray>
#include <QString>

#undef slots
#include <Python.h>
#define slots Q_SLOTS

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
