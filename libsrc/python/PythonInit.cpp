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
	#include <stdexcept>
#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

PythonInit::PythonInit()
{
	// register modules
	EffectModule::registerHyperionExtensionModule();

	// set Python module path when exists
	QString py_patch = QDir::cleanPath(qApp->applicationDirPath() + "/../lib/python");
	QString py_file  = QDir::cleanPath(qApp->applicationDirPath() + "/python" + STRINGIFY(PYTHON_VERSION_MAJOR_MINOR) + ".zip");

	if (QFile(py_file).exists() || QDir(py_patch).exists())
	{
		Py_NoSiteFlag++;
		if (QFile(py_file).exists())
			Py_SetPath(Py_DecodeLocale(py_file.toLatin1().data(), nullptr));
		else if (QDir(py_patch).exists())
			Py_SetPath(Py_DecodeLocale(py_patch.toLatin1().data(), nullptr));
	}

	// init Python
	Debug(Logger::getInstance("DAEMON"), "Initializing Python interpreter");
	Py_InitializeEx(0);
	if ( !Py_IsInitialized() )
	{
		throw std::runtime_error("Initializing Python failed!");
	}

#if (PY_VERSION_HEX < 0x03090000)
	// PyEval_InitThreads became deprecated in Python 3.9 and will be removed in Python 3.11
	PyEval_InitThreads(); // Create the GIL
#endif

	mainThreadState = PyEval_SaveThread();
}

PythonInit::~PythonInit()
{
	Debug(Logger::getInstance("DAEMON"), "Cleaning up Python interpreter");
	PyEval_RestoreThread(mainThreadState);
	Py_Finalize();
}
