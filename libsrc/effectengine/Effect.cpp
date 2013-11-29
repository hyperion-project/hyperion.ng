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


Effect::Effect(int priority, int timeout, const std::string & script, const std::string & args) :
	QThread(),
	_priority(priority),
	_timeout(timeout),
	_script(script),
	_args(args),
	_endTime(-1),
	_interpreterThreadState(nullptr),
	_abortRequested(false),
	_imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor())
{
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
	PyObject_SetAttrString(module, "args", Py_BuildValue("s", _args.c_str()));

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

void Effect::abort()
{
	_abortRequested = true;
}

void Effect::effectFinished()
{
	emit effectFinished(this);
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
			effect->setColors(effect->_priority, std::vector<ColorRgb>(effect->_imageProcessor->getLedCount(), color), timeout);
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
					std::vector<ColorRgb> colors(effect->_imageProcessor->getLedCount());
					char * data = PyByteArray_AS_STRING(bytearray);
					for (size_t i = 0; i < colors.size(); ++i)
					{
						ColorRgb & color = colors[i];
						color.red   = data [3*i];
						color.green = data [3*i+1];
						color.blue  = data [3*i+2];
					}
					effect->setColors(effect->_priority, colors, timeout);
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
				for (int y = 0; y < height; ++y)
				{
					for (int x = 0; x < width; ++x)
					{
						ColorRgb & color = image(x, y);
						int index = x+width*y;
						color.red   = data [3*index];
						color.green = data [3*index+1];
						color.blue  = data [3*index+2];
					}
				}

				std::vector<ColorRgb> colors(effect->_imageProcessor->getLedCount());
				effect->_imageProcessor->process(image, colors);
				effect->setColors(effect->_priority, colors, timeout);
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
	return Py_BuildValue("i", effect->_abortRequested ? 1 : 0);
}

Effect * Effect::getEffect(PyObject *self)
{
	// Get the effect from the capsule in the self pointer
	Effect * effect = reinterpret_cast<Effect *>(PyCapsule_GetPointer(self, nullptr));

	// Test if the effect has reached it end time
	if (effect->_timeout > 0 && QDateTime::currentMSecsSinceEpoch() > effect->_endTime)
	{
		effect->_abortRequested = true;
	}

	// return the effect
	return effect;
}
