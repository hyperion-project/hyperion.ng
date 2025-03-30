// Qt includes
#include <QDateTime>
#include <QFile>
#include <QResource>

// effect engin eincludes
#include <effectengine/Effect.h>
#include <effectengine/EffectModule.h>
#include <utils/Logger.h>
#include <hyperion/Hyperion.h>
#include <hyperion/PriorityMuxer.h>

// python utils
#include <python/PythonProgram.h>


// Constants
namespace {
	int DEFAULT_MAX_UPDATE_RATE_HZ { 200 };
} //End of constants

Effect::Effect(Hyperion *hyperion, int priority, int timeout, const QString &script, const QString &name, const QJsonObject &args, const QString &imageData)
	: QThread()
	, _hyperion(hyperion)
	, _priority(priority)
	, _timeout(timeout)
	, _isEndless(timeout <= PriorityMuxer::ENDLESS)
	, _script(script)
	, _name(name)
	, _args(args)
	, _imageData(imageData)
	, _endTime(-1)
	, _interupt(false)
	, _imageSize(hyperion->getLedGridSize())
	, _image(_imageSize,QImage::Format_ARGB32_Premultiplied)
	, _lowestUpdateIntervalInSeconds(1/static_cast<double>(DEFAULT_MAX_UPDATE_RATE_HZ))
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

bool Effect::isInterruptionRequested()
{
	return _interupt || (!_isEndless && getRemaining() <= 0);
}

int Effect::getRemaining() const
{
	// determine the timeout
	int timeout = _timeout;

	if (timeout >= 0)
	{
		timeout = static_cast<int>(_endTime - QDateTime::currentMSecsSinceEpoch());
	}
	return timeout;
}

bool Effect::setModuleParameters()
{
	// import the buildtin Hyperion module
	PyObject* module = PyImport_ImportModule("hyperion");

	if (module == nullptr) {
		PyErr_Print();  // Print error if module import fails
		return false;
	}

	// Add a capsule containing 'this' to the module for callback access
	PyObject* capsule = PyCapsule_New((void*)this, "hyperion.__effectObj", nullptr);
	if (capsule == nullptr || PyModule_AddObject(module, "__effectObj", capsule) < 0) {
		PyErr_Print();  // Print error if adding capsule fails
		Py_XDECREF(module);  // Clean up if capsule addition fails
		Py_XDECREF(capsule);
		return false;
	}

	// Add ledCount variable to the interpreter
	int ledCount = 0;
	QMetaObject::invokeMethod(_hyperion, "getLedCount", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, ledCount));
	PyObject* ledCountObj = Py_BuildValue("i", ledCount);
	if (PyObject_SetAttrString(module, "ledCount", ledCountObj) < 0) {
		PyErr_Print();  // Print error if setting attribute fails
	}
	Py_XDECREF(ledCountObj);

	// Add minimumWriteTime variable to the interpreter
	int latchTime = 0;
	QMetaObject::invokeMethod(_hyperion, "getLatchTime", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, latchTime));
	PyObject* latchTimeObj = Py_BuildValue("i", latchTime);
	if (PyObject_SetAttrString(module, "latchTime", latchTimeObj) < 0) {
		PyErr_Print();  // Print error if setting attribute fails
	}
	Py_XDECREF(latchTimeObj);

	// Add args variable to the interpreter
	PyObject* argsObj = EffectModule::json2python(_args);
	if (PyObject_SetAttrString(module, "args", argsObj) < 0) {
		PyErr_Print();  // Print error if setting attribute fails
	}
	Py_XDECREF(argsObj);

	// Decrement module reference
	Py_XDECREF(module);

	return true;
}

void Effect::run()
{
	PythonProgram program(_name, _log);

#if (PY_VERSION_HEX < 0x030C0000)
	PyThreadState* prev_thread_state = PyThreadState_Swap(program);
#endif

	if (!setModuleParameters())
	{
		Error(_log, "Failed to set Module parameters. Effect will not be executed.");
#if (PY_VERSION_HEX < 0x030C0000)
		PyThreadState_Swap(prev_thread_state);
#endif
		return;
	}

#if (PY_VERSION_HEX < 0x030C0000)
	PyThreadState_Swap(prev_thread_state);
#endif

	// Set the end time if applicable
	if (_timeout > 0)
	{
		_endTime = QDateTime::currentMSecsSinceEpoch() + _timeout;
	}

	// Run the effect script
	QFile file(_script);
	if (file.open(QIODevice::ReadOnly))
	{
		program.execute(file.readAll());
	}
	else
	{
		Error(_log, "Unable to open script file %s.", QSTRING_CSTR(_script));
	}
	file.close();
}

void Effect::stop()
{
	requestInterruption();
	Debug(_log,"Effect \"%s\" stopped", QSTRING_CSTR(_name));
}
