#pragma once

#include <QByteArray>
#include <QString>

#undef slots
// Don't use debug Python APIs on Windows
#if defined(_MSC_VER) && defined(_DEBUG)
#if _MSC_VER >= 1930
#include <corecrt.h>
#endif
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif
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
