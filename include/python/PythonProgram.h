#pragma once

#include <QByteArray>
#include <QString>

// Python includes
// collide of qt slots macro
#undef slots
#include "Python.h"
#define slots

class Logger;

class PythonProgram
{
public:
	PythonProgram(const QString & name, Logger * log);
	~PythonProgram();

	void execute(const QByteArray &python_code);

private:
	QString _name;
	Logger* _log;
	PyThreadState* _tstate;
};
