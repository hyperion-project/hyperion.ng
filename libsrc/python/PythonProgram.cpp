#include <python/PythonProgram.h>
#include <python/PythonUtils.h>

#include <utils/Logger.h>

#include <QThread>

PyThreadState* mainThreadState;

PythonProgram::PythonProgram(const QString& name, Logger* log) :
	_name(name)
	, _log(log)
	, _tstate(nullptr)
{
	// we probably need to wait until mainThreadState is available
	QThread::msleep(10);
	while (mainThreadState == nullptr)
	{
		QThread::msleep(10);  // Wait with delay to avoid busy waiting
	}

	// Create a new subinterpreter for this thread
#if (PY_VERSION_HEX < 0x030C0000)
	// get global lock
	PyEval_RestoreThread(mainThreadState);
	_tstate = Py_NewInterpreter();
#else
	PyThreadState* prev = PyThreadState_Swap(NULL);

	// Create a new interpreter configuration object
	PyInterpreterConfig config{};

	// Set configuration options
	config.use_main_obmalloc = 0;
	config.allow_fork = 0;
	config.allow_exec = 0;
	config.allow_threads = 1;
	config.allow_daemon_threads = 0;
	config.check_multi_interp_extensions = 1;
	config.gil = PyInterpreterConfig_OWN_GIL;
	Py_NewInterpreterFromConfig(&_tstate, &config);
#endif

	if (_tstate == nullptr)
	{
		PyThreadState_Swap(mainThreadState);
#if (PY_VERSION_HEX < 0x030C0000)
		PyEval_SaveThread();
#endif
		Error(_log, "Failed to get thread state for %s", QSTRING_CSTR(_name));
		return;
	}
#if (PY_VERSION_HEX < 0x030C0000)
	PyThreadState_Swap(_tstate);
#endif
}

PythonProgram::~PythonProgram()
{
	if (!_tstate)
	{
		return;
	}

#if (PY_VERSION_HEX < 0x030C0000)
	PyThreadState* prev_thread_state = PyThreadState_Swap(_tstate);
#endif

	// Clean up the thread state
	Py_EndInterpreter(_tstate);

#if (PY_VERSION_HEX < 0x030C0000)
	PyThreadState_Swap(prev_thread_state);
	PyEval_SaveThread();
#endif
}

void PythonProgram::execute(const QByteArray& python_code)
{
	if (!_tstate)
	{
		return;
	}

	PyThreadState* prev_thread_state = PyThreadState_Swap(_tstate);

	PyObject* main_module = PyImport_ImportModule("__main__");
	if (!main_module)
	{
		// Restore the previous thread state
#if (PY_VERSION_HEX < 0x030C0000)
		PyThreadState_Swap(mainThreadState);
#else
		PyThreadState_Swap(prev_thread_state);
#endif
		return;
	}

	PyObject* main_dict = PyModule_GetDict(main_module);  // Borrowed reference to globals
	PyObject* result = PyRun_String(python_code.constData(), Py_file_input, main_dict, main_dict);
	if (!result)
	{
		if (PyErr_Occurred())
		{
			Error(_log, "###### PYTHON EXCEPTION ######");
			Error(_log, "## In effect '%s'", QSTRING_CSTR(_name));

			PyObject* errorType = NULL, * errorValue = NULL, * errorTraceback = NULL;

			PyErr_Fetch(&errorType, &errorValue, &errorTraceback);
			PyErr_NormalizeException(&errorType, &errorValue, &errorTraceback);

			if (errorValue)
			{
				QString message;
				if (PyObject_HasAttrString(errorValue, "__class__"))
				{
					PyObject* classPtr = PyObject_GetAttrString(errorValue, "__class__");
					PyObject* class_name = classPtr ? PyObject_GetAttrString(classPtr, "__name__") : NULL;

					if (class_name && PyUnicode_Check(class_name))
						message.append(PyUnicode_AsUTF8(class_name));

					Py_XDECREF(class_name);
					Py_DECREF(classPtr);
				}

				PyObject* valueString = PyObject_Str(errorValue);

				if (valueString && PyUnicode_Check(valueString))
				{
					if (!message.isEmpty())
						message.append(": ");
					message.append(PyUnicode_AsUTF8(valueString));
				}
				Py_XDECREF(valueString);

				Error(_log, "## %s", QSTRING_CSTR(message));
			}

			if (errorTraceback)
			{
				PyObject* tracebackModule = PyImport_ImportModule("traceback");
				PyObject* methodName = PyUnicode_FromString("format_exception");
				PyObject* tracebackList = tracebackModule && methodName
					? PyObject_CallMethodObjArgs(tracebackModule, methodName, errorType, errorValue, errorTraceback, NULL)
					: NULL;

				if (tracebackList)
				{
					PyObject* iterator = PyObject_GetIter(tracebackList);
					PyObject* item;
					while ((item = PyIter_Next(iterator)))
					{
						Error(_log, "## %s", QSTRING_CSTR(QString(PyUnicode_AsUTF8(item)).trimmed()));
						Py_DECREF(item);
					}
					Py_DECREF(iterator);
				}

				Py_XDECREF(tracebackModule);
				Py_XDECREF(methodName);
				Py_XDECREF(tracebackList);

				PyErr_Restore(errorType, errorValue, errorTraceback);
			}
			Error(_log, "###### EXCEPTION END ######");
		}
	}
	else
	{
		Py_DECREF(result);  // Release result when done
	}

	Py_DECREF(main_module);

	// Restore the previous thread state
#if (PY_VERSION_HEX < 0x030C0000)
	PyThreadState_Swap(mainThreadState);
#else
	PyThreadState_Swap(prev_thread_state);
#endif
}

