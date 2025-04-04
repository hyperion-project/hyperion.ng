#pragma once

#include <QByteArray>
#include <QString>

#undef slots
#include <Python.h>
#define slots Q_SLOTS

#include <python/PythonUtils.h>

class Logger;

class PythonProgram
{
public:
	PythonProgram(const QString & name, Logger * log);
	~PythonProgram();

	operator PyThreadState* ()
	{
		return _tstate;
	}

	void execute(const QByteArray &python_code);

private:

	QString _name;
	Logger* _log;
	PyThreadState* _tstate;
};
