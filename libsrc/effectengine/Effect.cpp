// stl includes
#include <iostream>
#include <sstream>
#include <cmath>

// Qt includes
#include <QDateTime>
#include <QFile>
#include <Qt>
#include <QLinearGradient>
#include <QConicalGradient>
#include <QRadialGradient>
#include <QRect>
#include <QImageReader>
#include <QResource>

// effect engin eincludes
#include <effectengine/Effect.h>
#include <effectengine/EffectModule.h>
#include <utils/Logger.h>
#include <hyperion/Hyperion.h>

// python utils/ global mainthread
#include <python/PythonUtils.h>
//impl
PyThreadState* mainThreadState;

Effect::Effect(Hyperion* hyperion, int priority, int timeout, const QString & script, const QString & name, const QJsonObject & args)
	: QThread()
	, _hyperion(hyperion)
	, _priority(priority)
	, _timeout(timeout)
	, _script(script)
	, _name(name)
	, _args(args)
	, _endTime(-1)
	, _colors()
	, _imageSize(hyperion->getLedGridSize())
	, _image(_imageSize,QImage::Format_ARGB32_Premultiplied)
{
	_colors.resize(_hyperion->getLedCount());
	_colors.fill(ColorRgb::BLACK);

	_log = Logger::getInstance("EFFECTENGINE");

	// init effect image for image based effects, size is based on led layout
	_image.fill(Qt::black);
	_painter = new QPainter(&_image);

	Q_INIT_RESOURCE(EffectEngine);
}

Effect::~Effect()
{
	delete _painter;
	_imageStack.clear();
}

void Effect::run()
{
	// get global lock
	PyEval_RestoreThread(mainThreadState);

	// Initialize a new thread state
	PyThreadState* tstate = Py_NewInterpreter();
	if(tstate == nullptr)
	{
		PyEval_ReleaseLock();
		Error(_log, "Failed to get thread state for %s",QSTRING_CSTR(_name));
		return;
	}
	PyThreadState_Swap(tstate);

	// import the buildtin Hyperion module
	PyObject * module = PyImport_ImportModule("hyperion");

	// add a capsule containing 'this' to the module to be able to retrieve the effect from the callback function
	PyObject_SetAttrString(module, "__effectObj", PyCapsule_New(this, nullptr, nullptr));

	// add ledCount variable to the interpreter
	PyObject_SetAttrString(module, "ledCount", Py_BuildValue("i", _hyperion->getLedCount()));

	// add minimumWriteTime variable to the interpreter
	PyObject_SetAttrString(module, "latchTime", Py_BuildValue("i", _hyperion->getLatchTime()));

	// add a args variable to the interpreter
	PyObject_SetAttrString(module, "args", EffectModule::json2python(_args));

	// decref the module
	Py_XDECREF(module);

	// Set the end time if applicable
	if (_timeout > 0)
	{
		_endTime = QDateTime::currentMSecsSinceEpoch() + _timeout;
	}

	// Run the effect script
	QFile file (_script);
	QByteArray python_code;
	if (file.open(QIODevice::ReadOnly))
	{
		python_code = file.readAll();
	}
	else
	{
		Error(_log, "Unable to open script file %s.", QSTRING_CSTR(_script));
	}
	file.close();

	if (!python_code.isEmpty())
	{
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
	// stop sub threads if needed
	for (PyThreadState* s = tstate->interp->tstate_head, *old = nullptr; s;)
	{
		if (s == tstate)
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
		msleep(100);
		Py_END_ALLOW_THREADS;

		s = tstate->interp->tstate_head;
	}

	// Clean up the thread state
	Py_EndInterpreter(tstate);
	PyEval_ReleaseLock();
}
