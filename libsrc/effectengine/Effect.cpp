// stl includes
#include <iostream>
#include <sstream>

// effect engin eincludes
#include "Effect.h"

// Effect wrapper methods for Python interpreter extra build in methods
static PyObject* Effect_setColor(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyObject* Effect_setImage(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyObject* Effect_getLedCount(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyObject* Effect_abort(PyObject *self, PyObject *)
{
	Effect * effect = reinterpret_cast<Effect *>(PyCapsule_GetPointer(self, nullptr));
	bool abort = effect->isAbortedRequested();
	return Py_BuildValue("i", abort ? 1 : 0);
}

static PyMethodDef effectMethods[] = {
	{"setColor",    Effect_setColor,    METH_VARARGS, "Set a new color for the leds."},
	{"setImage",    Effect_setImage,    METH_VARARGS, "Set a new image to process and determine new led colors."},
	{"getLedCount", Effect_getLedCount, METH_VARARGS, "Get the number of avaliable led channels."},
	{"abort",       Effect_abort,       METH_NOARGS , "Check if the effect should abort execution."},
	{NULL, NULL, 0, NULL}
};


Effect::Effect(int priority, int timeout) :
	QThread(),
	_priority(priority),
	_timeout(timeout),
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
	std::cout << this << std::endl;

	// Run the effect script
	std::string script = "test.py";
	FILE* file = fopen(script.c_str(), "r");
	PyRun_SimpleFile(file, script.c_str());

	// Clean up the thread state
	Py_EndInterpreter(_interpreterThreadState);
	_interpreterThreadState = nullptr;
	PyEval_ReleaseLock();
}

bool Effect::isAbortedRequested() const
{
	return _abortRequested;
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
