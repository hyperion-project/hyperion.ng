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
	{"getLedCount", Effect::wrapGetLedCount, METH_VARARGS, "Get the number of avaliable led channels."},
	{"abort",       Effect::wrapAbort,       METH_NOARGS , "Check if the effect should abort execution."},
	{NULL, NULL, 0, NULL}
};


Effect::Effect(int priority, int timeout) :
	QThread(),
	_priority(priority),
	_timeout(timeout),
	_endTime(-1),
	_interpreterThreadState(nullptr),
	_abortRequested(false)
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
	Py_InitModule4("hyperion", effectMethods, nullptr, thisCapsule, PYTHON_API_VERSION);

	// Set the end time if applicable
	if (_timeout > 0)
	{
		_endTime = QDateTime::currentMSecsSinceEpoch() + _timeout;
	}

	// Run the effect script
	std::string script = "test.py";
	FILE* file = fopen(script.c_str(), "r");
	PyRun_SimpleFile(file, script.c_str());

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
	Effect * effect = getEffect(self);
	return Py_BuildValue("i", 42);
}

PyObject* Effect::wrapSetImage(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect(self);
	return Py_BuildValue("i", 42);
}

PyObject* Effect::wrapGetLedCount(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect(self);
	return Py_BuildValue("i", 42);
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
