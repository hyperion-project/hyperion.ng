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

#ifdef _WIN32
	#define STRINGIFY2(x) #x
	#define STRINGIFY(x) STRINGIFY2(x)
#endif

PythonInit::PythonInit()
{
	// register modules
	EffectModule::registerHyperionExtensionModule();

	// set Python module path when exists
	wchar_t *pythonPath = Py_DecodeLocale((QDir::cleanPath(qApp->applicationDirPath() + "/../lib/python")).toLatin1().data(), nullptr);
	#ifdef _WIN32
		pythonPath = Py_DecodeLocale((QDir::cleanPath(qApp->applicationDirPath())).toLatin1().data(), nullptr);
		pythonPath =  wcscat(pythonPath, L"/python" STRINGIFY(PYTHON_VERSION_MAJOR_MINOR) ".zip");
		if(QFile(QString::fromWCharArray(pythonPath)).exists())
	#else
		if(QDir(QString::fromWCharArray(pythonPath)).exists())
	#endif

	{
		Py_NoSiteFlag++;
		Py_SetPath(pythonPath);
		PyMem_RawFree(pythonPath);
	}

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
