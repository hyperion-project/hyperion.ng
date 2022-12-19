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

#if (PY_VERSION_HEX >= 0x03080000)
	PyStatus status;
	PyConfig config;
	PyConfig_InitPythonConfig(&config);
#endif

#if defined(ENABLE_DEPLOY_DEPENDENCIES)

	Debug(Logger::getInstance("DAEMON"), "Initializing Python config");
	// Set Program name
	wchar_t programName[] = L"Hyperion";

#if (PY_VERSION_HEX >= 0x03080000)
	status = PyConfig_SetString(&config, &config.program_name, programName);
	if (PyStatus_Exception(status)) {
		goto exception;
	}
	else
#else
	Py_SetProgramName(programName);
#endif
	{
		// set Python module path when exists
		QString py_path = QDir::cleanPath(qApp->applicationDirPath() + "/../lib/python" + STRINGIFY(PYTHON_VERSION_MAJOR) + "." + STRINGIFY(PYTHON_VERSION_MINOR));
		QString py_file = QDir::cleanPath(qApp->applicationDirPath() + "/python" + STRINGIFY(PYTHON_VERSION_MAJOR) + STRINGIFY(PYTHON_VERSION_MINOR) + ".zip");
		QString py_framework = QDir::cleanPath(qApp->applicationDirPath() + "/../Frameworks/Python.framework/Versions/Current/lib/python" + STRINGIFY(PYTHON_VERSION_MAJOR) + "." + STRINGIFY(PYTHON_VERSION_MINOR));

		if (QFile(py_file).exists() || QDir(py_path).exists() || QDir(py_framework).exists())
		{
			Py_NoSiteFlag++;
			if (QFile(py_file).exists()) // Windows
			{
#if (PY_VERSION_HEX >= 0x03080000)
				status = PyConfig_SetBytesString(&config, &config.home, QSTRING_CSTR(py_file));
				if (PyStatus_Exception(status)) {
					goto exception;
				}
				config.module_search_paths_set = 1;
				status = PyWideStringList_Append(&config.module_search_paths, const_cast<wchar_t*>(py_file.toStdWString().c_str()));
				if (PyStatus_Exception(status)) {
					goto exception;
				}
#else
				Py_SetPythonHome(Py_DecodeLocale(py_file.toLatin1().data(), nullptr));
				Py_SetPath(Py_DecodeLocale(py_file.toLatin1().data(), nullptr));
#endif
			}
			else if (QDir(py_path).exists()) // Linux
			{
#if (PY_VERSION_HEX >= 0x03080000)
				status = PyConfig_SetBytesString(&config, &config.home, QSTRING_CSTR(QDir::cleanPath(qApp->applicationDirPath() + "/../")));
				if (PyStatus_Exception(status)) {
					goto exception;
				}

				config.module_search_paths_set = 1;
				status = PyWideStringList_Append(&config.module_search_paths, const_cast<wchar_t*>(QDir(py_path).absolutePath().toStdWString().c_str()));
				if (PyStatus_Exception(status)) {
					goto exception;
				}

				status = PyWideStringList_Append(&config.module_search_paths, const_cast<wchar_t*>(QDir(py_path + "/lib-dynload").absolutePath().toStdWString().c_str()));
				if (PyStatus_Exception(status)) {
					goto exception;
				}
#else
				QStringList python_paths;
				python_paths.append(QDir(py_path).absolutePath());
				python_paths.append(QDir(py_path + "/lib-dynload").absolutePath());
				QVector<wchar_t> joined_paths(python_paths.join(":").size() + 1, 0);
				python_paths.join(":").toWCharArray(joined_paths.data());
				Py_SetPath(joined_paths.data());
				py_path = QDir::cleanPath(qApp->applicationDirPath() + "/../");
				Py_SetPythonHome(Py_DecodeLocale(py_path.toLatin1().data(), nullptr));
#endif
			}
			else if (QDir(py_framework).exists()) // macOS
			{
#if (PY_VERSION_HEX >= 0x03080000)
				status = PyConfig_SetBytesString(&config, &config.home, QSTRING_CSTR(QDir::cleanPath(qApp->applicationDirPath() + "/../Frameworks/Python.framework/Versions/Current")));
				if (PyStatus_Exception(status)) {
					goto exception;
				}

				config.module_search_paths_set = 1;
				status = PyWideStringList_Append(&config.module_search_paths, const_cast<wchar_t*>(QDir(py_framework).absolutePath().toStdWString().c_str()));
				if (PyStatus_Exception(status)) {
					goto exception;
				}

				status = PyWideStringList_Append(&config.module_search_paths, const_cast<wchar_t*>(QDir(py_framework + "/lib-dynload").absolutePath().toStdWString().c_str()));
				if (PyStatus_Exception(status)) {
					goto exception;
				}
#else
				QStringList python_paths;
				python_paths.append(QDir(py_framework).absolutePath());
				python_paths.append(QDir(py_framework + "/lib-dynload").absolutePath());
				QVector<wchar_t> joined_paths(python_paths.join(":").size() + 1, 0);
				python_paths.join(":").toWCharArray(joined_paths.data());
				Py_SetPath(joined_paths.data());
				py_framework = QDir::cleanPath(qApp->applicationDirPath() + "/../Frameworks/Python.framework/Versions/Current");
				Py_SetPythonHome(Py_DecodeLocale(py_framework.toLatin1().data(), nullptr));
#endif
			}
		}
	}

#endif

#if (PY_VERSION_HEX >= 0x03080000)
	status = Py_InitializeFromConfig(&config);
	if (PyStatus_Exception(status)) {
		goto exception;
	}
	PyConfig_Clear(&config);
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
	return;

#if (PY_VERSION_HEX >= 0x03080000)
exception:
	Error(Logger::getInstance("DAEMON"), "Initializing Python config failed with error [%s]", status.err_msg);
	PyConfig_Clear(&config);

	throw std::runtime_error("Initializing Python failed!");
#endif
}

PythonInit::~PythonInit()
{
	Debug(Logger::getInstance("DAEMON"), "Cleaning up Python interpreter");
	PyEval_RestoreThread(mainThreadState);
	Py_Finalize();
}
