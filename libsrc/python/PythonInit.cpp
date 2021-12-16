// utils
#include <utils/Logger.h>

#include <python/PythonInit.h>
#include <python/PythonUtils.h>

// qt include
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QVector>
#include <QStringList>

// modules to init
#include <effectengine/EffectModule.h>

// Required to determine the cmake options
#include <HyperionConfig.h>

#ifdef _WIN32
	#include <stdexcept>
#endif

#define STRINGIFY2(x) #x
#define STRINGIFY(x) STRINGIFY2(x)

PythonInit::PythonInit()
{
	// register modules
	EffectModule::registerHyperionExtensionModule();

#if defined(ENABLE_DEPLOY_DEPENDENCIES)
	// Set Program name
	wchar_t programName[] = L"Hyperion";
	Py_SetProgramName(programName);

	// set Python module path when exists
	QString py_path = QDir::cleanPath(qApp->applicationDirPath() + "/../lib/python" + STRINGIFY(PYTHON_VERSION_MAJOR) + "." + STRINGIFY(PYTHON_VERSION_MINOR));
	QString py_file = QDir::cleanPath(qApp->applicationDirPath() + "/python" + STRINGIFY(PYTHON_VERSION_MAJOR) + STRINGIFY(PYTHON_VERSION_MINOR) + ".zip");
	QString py_framework = QDir::cleanPath(qApp->applicationDirPath() + "/../Frameworks/Python.framework/Versions/Current/lib/python" + STRINGIFY(PYTHON_VERSION_MAJOR) + "." + STRINGIFY(PYTHON_VERSION_MINOR));

	if (QFile(py_file).exists() || QDir(py_path).exists() || QDir(py_framework).exists() )
	{
		Py_NoSiteFlag++;
		if (QFile(py_file).exists()) // Windows
		{
			Py_SetPythonHome(Py_DecodeLocale(py_file.toLatin1().data(), nullptr));
			Py_SetPath(Py_DecodeLocale(py_file.toLatin1().data(), nullptr));
		}
		else if (QDir(py_path).exists()) // Linux
		{
			QStringList python_paths;
			python_paths.append(QDir(py_path).absolutePath());
			python_paths.append(QDir(py_path + "/lib-dynload").absolutePath());
			QVector<wchar_t> joined_paths(python_paths.join(":").size() + 1, 0);
			python_paths.join(":").toWCharArray(joined_paths.data());
			Py_SetPath(joined_paths.data());
			py_path = QDir::cleanPath(qApp->applicationDirPath() + "/../");
			Py_SetPythonHome(Py_DecodeLocale(py_path.toLatin1().data(), nullptr));
		}
		else if (QDir(py_framework).exists()) // macOS
		{
			QStringList python_paths;
			python_paths.append(QDir(py_framework).absolutePath());
			python_paths.append(QDir(py_framework + "/lib-dynload").absolutePath());
			QVector<wchar_t> joined_paths(python_paths.join(":").size() + 1, 0);
			python_paths.join(":").toWCharArray(joined_paths.data());
			Py_SetPath(joined_paths.data());
			py_framework = QDir::cleanPath(qApp->applicationDirPath() + "/../Frameworks/Python.framework/Versions/Current");
			Py_SetPythonHome(Py_DecodeLocale(py_framework.toLatin1().data(), nullptr));
		}
	}
#endif

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
