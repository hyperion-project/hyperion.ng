// stl includes
#include <sstream>

// effect engin eincludes
#include "Effect.h"

// Effect wrapper methods for Python interpreter extra build in methods
static PyObject* Effect_SetColor(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyObject* Effect_SetImage(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyObject* Effect_GetLedCount(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyObject* Effect_IsAbortRequested(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", 42);
}

static PyMethodDef effectMethods[] = {
	{"setColor",         Effect_SetColor,         METH_VARARGS, "Set a new color for the leds."},
	{"setImage",         Effect_SetImage,         METH_VARARGS, "Set a new image to process and determine new led colors."},
	{"getLedCount",      Effect_GetLedCount,      METH_VARARGS, "Get the number of avaliable led channels."},
	{"isAbortRequested", Effect_IsAbortRequested, METH_VARARGS, "Check if the effect should abort execution."},
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
	Py_InitModule("hyperiond", effectMethods);

	// Create hyperion instance in the new interpreter
	std::ostringstream oss;
	oss << "import hyperiond"                              << std::endl;
	oss << "class Hyperion:"                               << std::endl;
	oss << "    def setColor(self):"                       << std::endl;
	oss << "        return hyperiond.setColor()"           << std::endl;
	oss << "    def setImage(self):"                       << std::endl;
	oss << "        return hyperiond.setImage()"           << std::endl;
	oss << "    def getLedCount(self):"                    << std::endl;
	oss << "        return hyperiond.getLedCount()"        << std::endl;
	oss << "    def isAbortRequested(self):"               << std::endl;
	oss << "        return hyperiond.isAbortRequested()"   << std::endl;
	oss << "hyperion = Hyperion()"                         << std::endl;
	PyRun_SimpleString(oss.str().c_str());

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
