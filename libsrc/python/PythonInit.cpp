#undef slots
#include <Python.h>
#define slots

// utils
#include <utils/Logger.h>

#include <python/PythonInit.h>
#include <python/PythonUtils.h>

// qt include
#include <QCoreApplication>
#include <QDir>

// modules to init
#include <effectengine/EffectModule.h>

PythonInit::PythonInit()
{
	// register modules
	EffectModule::registerHyperionExtensionModule();

	// set Python module path when exists
	// TODO Crash on Windows
#ifndef _WIN32
	const wchar_t *pythonPath = Py_DecodeLocale((QDir::cleanPath(qApp->applicationDirPath() + "/../lib/python")).toLatin1().data(), nullptr);
	if(QDir(QString::fromWCharArray(pythonPath)).exists())
	{
		Py_NoSiteFlag++;
		Py_SetPath(pythonPath);
	}
	delete pythonPath;

#endif
	// init Python
	Debug(Logger::getInstance("DAEMON"), "Initializing Python interpreter");
	Py_InitializeEx(0);
	if ( !Py_IsInitialized() )
	{
		throw std::runtime_error("Initializing Python failed!");
	}

	PyEval_InitThreads(); // Create the GIL
	mainThreadState = PyEval_SaveThread();
}

PythonInit::~PythonInit()
{
	Debug(Logger::getInstance("DAEMON"), "Cleaning up Python interpreter");
	PyEval_RestoreThread(mainThreadState);
	Py_Finalize();
}
