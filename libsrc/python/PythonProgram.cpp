#include <python/PythonProgram.h>
#include <python/PythonUtils.h>
#include <utils/Logger.h>

#include <QThread>

PyThreadState* mainThreadState;

PythonProgram::PythonProgram(const QString & name, Logger * log) :
	_name(name), _log(log), _tstate(nullptr)
{
	// we probably need to wait until mainThreadState is available
	while(mainThreadState == nullptr){};

	// get global lock
	PyEval_RestoreThread(mainThreadState);

	// Initialize a new thread state
	_tstate = Py_NewInterpreter();
	if(_tstate == nullptr)
	{
#if (PY_VERSION_HEX >= 0x03020000)
		PyThreadState_Swap(mainThreadState);
		PyEval_SaveThread();
#else
		PyEval_ReleaseLock();
#endif
		Error(_log, "Failed to get thread state for %s",QSTRING_CSTR(_name));
		return;
	}
	PyThreadState_Swap(_tstate);
}

PythonProgram::~PythonProgram()
{
	if (!_tstate)
		return;

	// stop sub threads if needed
	for (PyThreadState* s = PyInterpreterState_ThreadHead(_tstate->interp), *old = nullptr; s;)
	{
		if (s == _tstate)
		{
			s = s->next;
			continue;
		}
		if (old != s)
		{
			Debug(_log,"ID %s: Waiting on thread %u", QSTRING_CSTR(_name), s->thread_id);
			old = s;
		}

		Py_BEGIN_ALLOW_THREADS;
		QThread::msleep(100);
		Py_END_ALLOW_THREADS;

		s = PyInterpreterState_ThreadHead(_tstate->interp);
	}

	// Clean up the thread state
	Py_EndInterpreter(_tstate);
#if (PY_VERSION_HEX >= 0x03020000)
	PyThreadState_Swap(mainThreadState);
	PyEval_SaveThread();
#else
	PyEval_ReleaseLock();
#endif
}

void PythonProgram::execute(const QByteArray & python_code)
{
	if (!_tstate)
		return;

	PyObject *main_module = PyImport_ImportModule("__main__"); // New Reference
	PyObject *main_dict = PyModule_GetDict(main_module); // Borrowed reference
	Py_INCREF(main_dict); // Incref "main_dict" to use it in PyRun_String(), because PyModule_GetDict() has decref "main_dict"
	Py_DECREF(main_module); // // release "main_module" when done
	PyObject *result = PyRun_String(python_code.constData(), Py_file_input, main_dict, main_dict); // New Reference

	if (!result)
	{
		if (PyErr_Occurred()) // Nothing needs to be done for a borrowed reference
		{
			Error(_log,"###### PYTHON EXCEPTION ######");
			Error(_log,"## In effect '%s'", QSTRING_CSTR(_name));
			/* Objects all initialized to NULL for Py_XDECREF */
			PyObject *errorType = NULL, *errorValue = NULL, *errorTraceback = NULL;

			PyErr_Fetch(&errorType, &errorValue, &errorTraceback); // New Reference or NULL
			PyErr_NormalizeException(&errorType, &errorValue, &errorTraceback);

			// Extract exception message from "errorValue"
			if(errorValue)
			{
				QString message;
				if(PyObject_HasAttrString(errorValue, "__class__"))
				{
					PyObject *classPtr = PyObject_GetAttrString(errorValue, "__class__"); // New Reference
					PyObject *class_name = NULL; /* Object "class_name" initialized to NULL for Py_XDECREF */
					class_name = PyObject_GetAttrString(classPtr, "__name__"); // New Reference or NULL

					if(class_name && PyUnicode_Check(class_name))
						message.append(PyUnicode_AsUTF8(class_name));

					Py_DECREF(classPtr); // release "classPtr" when done
					Py_XDECREF(class_name); // Use Py_XDECREF() to ignore NULL references
				}

				// Object "class_name" initialized to NULL for Py_XDECREF
				PyObject *valueString = NULL;
				valueString = PyObject_Str(errorValue); // New Reference or NULL

				if(valueString && PyUnicode_Check(valueString))
				{
					if(!message.isEmpty())
						message.append(": ");

					message.append(PyUnicode_AsUTF8(valueString));
				}
				Py_XDECREF(valueString); // Use Py_XDECREF() to ignore NULL references

				Error(_log, "## %s", QSTRING_CSTR(message));
			}

			// Extract exception message from "errorTraceback"
			if(errorTraceback)
			{
				// Object "tracebackList" initialized to NULL for Py_XDECREF
				PyObject *tracebackModule = NULL, *methodName = NULL, *tracebackList = NULL;
				QString tracebackMsg;

				tracebackModule = PyImport_ImportModule("traceback"); // New Reference or NULL
				methodName = PyUnicode_FromString("format_exception"); // New Reference or NULL
				tracebackList = PyObject_CallMethodObjArgs(tracebackModule, methodName, errorType, errorValue, errorTraceback, NULL); // New Reference or NULL

				if(tracebackList)
				{
					PyObject* iterator = PyObject_GetIter(tracebackList); // New Reference

					PyObject* item;
					while( (item = PyIter_Next(iterator)) ) // New Reference
					{
						Error(_log, "## %s",QSTRING_CSTR(QString(PyUnicode_AsUTF8(item)).trimmed()));
						Py_DECREF(item); // release "item" when done
					}
					Py_DECREF(iterator);  // release "iterator" when done
				}

				// Use Py_XDECREF() to ignore NULL references
				Py_XDECREF(tracebackModule);
				Py_XDECREF(methodName);
				Py_XDECREF(tracebackList);

				// Give the exception back to python and print it to stderr in case anyone else wants it.
				Py_XINCREF(errorType);
				Py_XINCREF(errorValue);
				Py_XINCREF(errorTraceback);

				PyErr_Restore(errorType, errorValue, errorTraceback);
				//PyErr_PrintEx(0); // Remove this line to switch off stderr output
			}
			Error(_log,"###### EXCEPTION END ######");
		}
	}
	else
	{
		Py_DECREF(result);  // release "result" when done
	}

	Py_DECREF(main_dict);  // release "main_dict" when done
}

