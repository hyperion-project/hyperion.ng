// python utils (must be included first to avoid Python header issues)
#include <effectengine/EffectModule.h>
#include <python/PythonProgram.h>

// Qt includes
#include <QDateTime>
#include <QFile>
#include <QResource>

// effect engine includes
#include <effectengine/Effect.h>
#include <utils/Logger.h>
#include <hyperion/Hyperion.h>
#include <hyperion/PriorityMuxer.h>


// Constants
namespace {
	constexpr int DEFAULT_MAX_UPDATE_RATE_HZ { 200 };
} //End of constants

Effect::Effect(const QSharedPointer<Hyperion>& hyperionInstance, int priority, int timeout, const QString &script, const QString &name, const QJsonObject &args, const QString &imageData)
	: QThread()
	, _hyperionWeak(hyperionInstance)
	, _priority(priority)
	, _timeout(timeout)
	, _isEndless(timeout <= PriorityMuxer::ENDLESS)
	, _script(script)
	, _name(name)
	, _args(args)
	, _imageData(imageData)
	, _endTime(-1)
	, _interupt(false)
	, _imageSize()
	, _image()
	, _lowestUpdateIntervalInSeconds(1/static_cast<double>(DEFAULT_MAX_UPDATE_RATE_HZ))
{
	QString subComponent{ "__" };

	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();
	if (hyperion)
	{
		subComponent = hyperion->property("instance").toString();
		_colors.resize(hyperion->getLedCount());
		_imageSize = hyperion->getLedGridSize();
	}

	_log = Logger::getInstance("EFFECT", subComponent);
	TRACK_SCOPE_SUBCOMPONENT() << _name;

	_colors.fill(ColorRgb::BLACK);

	// init effect image for image based effects, size is based on led layout
	_image = QImage(_imageSize, QImage::Format_ARGB32_Premultiplied);
	_image.fill(Qt::black);
	_painter.reset(new QPainter(&_image));

	Q_INIT_RESOURCE(EffectEngine);
}

Effect::~Effect()
{
	TRACK_SCOPE_SUBCOMPONENT() << _name;
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
	PyObject* hyperionModule = PyImport_ImportModule("hyperion");

	if (hyperionModule == nullptr) {
		PyErr_Print();  // Print error if module import fails
		return false;
	}

	// Add a capsule containing 'this' to the module for callback access
	PyObject* capsule = PyCapsule_New((void*)this, "hyperion.__effectObj", nullptr);
	if (capsule == nullptr) {
		PyErr_Print();  // Print error if capsule creation fails
		Py_DECREF(hyperionModule);
		return false;
	}
#if (PY_VERSION_HEX >= 0x030A0000)
	// 3.10+: AddObjectRef never steals; module dict acquires its own ref internally.
	// Caller always owns capsule and must Py_DECREF regardless of success or failure.
	const int addResult = PyModule_AddObjectRef(hyperionModule, "__effectObj", capsule);
	Py_DECREF(capsule);
	if (addResult < 0) {
#else
	// Pre-3.10: AddObject steals the reference unconditionally (success AND failure).
	// Do not Py_DECREF capsule in either branch.
	if (PyModule_AddObject(hyperionModule, "__effectObj", capsule) < 0) {
#endif
		PyErr_Print();  // Print error if adding capsule fails
		Py_DECREF(hyperionModule);
		return false;
	}

	QSharedPointer<Hyperion> hyperion = _hyperionWeak.toStrongRef();

	// Add ledCount variable to the interpreter
	int ledCount = 0;
	QMetaObject::invokeMethod(hyperion.get(), "getLedCount", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, ledCount));
	PyObject* ledCountObj = Py_BuildValue("i", ledCount);
	if (ledCountObj == nullptr || PyObject_SetAttrString(hyperionModule, "ledCount", ledCountObj) < 0) {
		PyErr_Print();  // Print error if setting attribute fails
	}
	Py_XDECREF(ledCountObj);

	// Add minimumWriteTime variable to the interpreter
	int latchTime = 0;
	QMetaObject::invokeMethod(hyperion.get(), "getLatchTime", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, latchTime));
	PyObject* latchTimeObj = Py_BuildValue("i", latchTime);
	if (latchTimeObj == nullptr || PyObject_SetAttrString(hyperionModule, "latchTime", latchTimeObj) < 0) {
		PyErr_Print();  // Print error if setting attribute fails
	}
	Py_XDECREF(latchTimeObj);

	// Add args variable to the interpreter
	PyObject* argsObj = EffectModule::json2python(_args);
	if (argsObj == nullptr || PyObject_SetAttrString(hyperionModule, "args", argsObj) < 0) {
		PyErr_Print();  // Print error if conversion or setting attribute fails
	}
	Py_XDECREF(argsObj);

	// Decrement module reference (non-null guaranteed by null-check at top of function)
	Py_DECREF(hyperionModule);

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
