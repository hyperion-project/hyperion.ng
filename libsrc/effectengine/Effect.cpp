// Python include
#include <Python.h>

// stl includes
#include <iostream>
#include <sstream>

// Qt includes
#include <QDateTime>

// effect engin eincludes
#include "Effect.h"

// Python method table
PyMethodDef Effect::effectMethods[] = {
	{"setColor",    Effect::wrapSetColor,    METH_VARARGS, "Set a new color for the leds."},
	{"setImage",    Effect::wrapSetImage,    METH_VARARGS, "Set a new image to process and determine new led colors."},
	{"abort",       Effect::wrapAbort,       METH_NOARGS,  "Check if the effect should abort execution."},
	{NULL, NULL, 0, NULL}
};


Effect::Effect(int priority, int timeout, const std::string & script, const Json::Value & args) :
	QThread(),
	_priority(priority),
	_timeout(timeout),
	_script(script),
	_args(args),
	_endTime(-1),
	_interpreterThreadState(nullptr),
	_abortRequested(false),
	_imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_colors()
{
	_colors.resize(_imageProcessor->getLedCount(), ColorRgb::BLACK);

	// connect the finished signal
	connect(this, SIGNAL(finished()), this, SLOT(effectFinished()));
}

Effect::~Effect()
{
}

void Effect::run()
{
	// Initialize a new thread state
	PyEval_AcquireLock(); // Get the GIL
	_interpreterThreadState = Py_NewInterpreter();

	// add methods extra builtin methods to the interpreter
	PyObject * thisCapsule = PyCapsule_New(this, nullptr, nullptr);
	PyObject * module = Py_InitModule4("hyperion", effectMethods, nullptr, thisCapsule, PYTHON_API_VERSION);

	// add ledCount variable to the interpreter
	PyObject_SetAttrString(module, "ledCount", Py_BuildValue("i", _imageProcessor->getLedCount()));

	// add a args variable to the interpreter
	PyObject_SetAttrString(module, "args", json2python(_args));
	//PyObject_SetAttrString(module, "args", Py_BuildValue("s", _args.c_str()));

	// Set the end time if applicable
	if (_timeout > 0)
	{
		_endTime = QDateTime::currentMSecsSinceEpoch() + _timeout;
	}

	// Run the effect script
	FILE* file = fopen(_script.c_str(), "r");
	if (file != nullptr)
	{
		PyRun_SimpleFile(file, _script.c_str());
	}
	else
	{
		std::cerr << "Unable to open script file " << _script << std::endl;
	}

	// Clean up the thread state
	Py_EndInterpreter(_interpreterThreadState);
	_interpreterThreadState = nullptr;
	PyEval_ReleaseLock();
}

int Effect::getPriority() const
{
	return _priority;
}

bool Effect::isAbortRequested() const
{
	return _abortRequested;
}

void Effect::abort()
{
	_abortRequested = true;
}

void Effect::effectFinished()
{
	emit effectFinished(this);
}

PyObject *Effect::json2python(const Json::Value &json) const
{
	switch (json.type())
	{
	case Json::nullValue:
		return Py_BuildValue("");
	case Json::realValue:
		return Py_BuildValue("d", json.asDouble());
	case Json::intValue:
	case Json::uintValue:
		return Py_BuildValue("i", json.asInt());
	case Json::booleanValue:
		return Py_BuildValue("i", json.asBool() ? 1 : 0);
	case Json::stringValue:
		return Py_BuildValue("s", json.asCString());
	case Json::objectValue:
	{
		PyObject * obj = PyDict_New();
		for (Json::Value::iterator i = json.begin(); i != json.end(); ++i)
		{
			PyDict_SetItemString(obj, i.memberName(), json2python(*i));
		}
		return obj;
	}
	case Json::arrayValue:
	{
		PyObject * list = PyList_New(json.size());
		for (Json::Value::iterator i = json.begin(); i != json.end(); ++i)
		{
			PyList_SetItem(list, i.index(), json2python(*i));
		}
		return list;
	}
	}

	assert(false);
	return nullptr;
}

PyObject* Effect::wrapSetColor(PyObject *self, PyObject *args)
{
	// get the effect
	Effect * effect = getEffect(self);

	// check if we have aborted already
	if (effect->_abortRequested)
	{
		return Py_BuildValue("");
	}

	// determine the timeout
	int timeout = effect->_timeout;
	if (timeout > 0)
	{
		timeout = effect->_endTime - QDateTime::currentMSecsSinceEpoch();

		// we are done if the time has passed
		if (timeout <= 0)
		{
			return Py_BuildValue("");
		}
	}

	// check the number of arguments
	int argCount = PyTuple_Size(args);
	if (argCount == 3)
	{
		// three seperate arguments for red, green, and blue
		ColorRgb color;
		if (PyArg_ParseTuple(args, "bbb", &color.red, &color.green, &color.blue))
		{
			std::fill(effect->_colors.begin(), effect->_colors.end(), color);
			effect->setColors(effect->_priority, effect->_colors, timeout);
			return Py_BuildValue("");
		}
		else
		{
			return nullptr;
		}
	}
	else if (argCount == 1)
	{
		// bytearray of values
		PyObject * bytearray = nullptr;
		if (PyArg_ParseTuple(args, "O", &bytearray))
		{
			if (PyByteArray_Check(bytearray))
			{
				size_t length = PyByteArray_Size(bytearray);
				if (length == 3 * effect->_imageProcessor->getLedCount())
				{
					char * data = PyByteArray_AS_STRING(bytearray);
					memcpy(effect->_colors.data(), data, length);
					effect->setColors(effect->_priority, effect->_colors, timeout);
					return Py_BuildValue("");
				}
				else
				{
					PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should be 3*ledCount");
					return nullptr;
				}
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Argument is not a bytearray");
				return nullptr;
			}
		}
		else
		{
			return nullptr;
		}
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeError, "Function expect 1 or 3 arguments");
		return nullptr;
	}

	// error
	PyErr_SetString(PyExc_RuntimeError, "Unknown error");
	return nullptr;
}

PyObject* Effect::wrapSetImage(PyObject *self, PyObject *args)
{
	// get the effect
	Effect * effect = getEffect(self);

	// check if we have aborted already
	if (effect->_abortRequested)
	{
		return Py_BuildValue("");
	}

	// determine the timeout
	int timeout = effect->_timeout;
	if (timeout > 0)
	{
		timeout = effect->_endTime - QDateTime::currentMSecsSinceEpoch();

		// we are done if the time has passed
		if (timeout <= 0)
		{
			return Py_BuildValue("");
		}
	}

	// bytearray of values
	int width, height;
	PyObject * bytearray = nullptr;
	if (PyArg_ParseTuple(args, "iiO", &width, &height, &bytearray))
	{
		if (PyByteArray_Check(bytearray))
		{
			int length = PyByteArray_Size(bytearray);
			if (length == 3 * width * height)
			{
				Image<ColorRgb> image(width, height);
				char * data = PyByteArray_AS_STRING(bytearray);
				memcpy(image.memptr(), data, length);

				effect->_imageProcessor->process(image, effect->_colors);
				effect->setColors(effect->_priority, effect->_colors, timeout);
				return Py_BuildValue("");
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should be 3*ledCount");
				return nullptr;
			}
		}
		else
		{
			PyErr_SetString(PyExc_RuntimeError, "Argument 3 is not a bytearray");
			return nullptr;
		}
	}
	else
	{
		return nullptr;
	}

	// error
	PyErr_SetString(PyExc_RuntimeError, "Unknown error");
	return nullptr;
}

PyObject* Effect::wrapAbort(PyObject *self, PyObject *)
{
	Effect * effect = getEffect(self);

	// Test if the effect has reached it end time
	if (effect->_timeout > 0 && QDateTime::currentMSecsSinceEpoch() > effect->_endTime)
	{
		effect->_abortRequested = true;
	}

	return Py_BuildValue("i", effect->_abortRequested ? 1 : 0);
}

Effect * Effect::getEffect(PyObject *self)
{
	// Get the effect from the capsule in the self pointer
	return reinterpret_cast<Effect *>(PyCapsule_GetPointer(self, nullptr));
}
