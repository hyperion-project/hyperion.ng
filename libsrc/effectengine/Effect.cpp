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
	requestInterruption();
	wait();

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
		timeout = static_cast<int>( _endTime - QDateTime::currentMSecsSinceEpoch());
	}
	return timeout;
}

void Effect::setModuleParameters()
{
	// import the buildtin Hyperion module
	PyObject * module = PyImport_ImportModule("hyperion");

	// add a capsule containing 'this' to the module to be able to retrieve the effect from the callback function
	PyModule_AddObject(module, "__effectObj", PyCapsule_New((void*)this, "hyperion.__effectObj", nullptr));

	// add ledCount variable to the interpreter
	int ledCount = 0;
	QMetaObject::invokeMethod(_hyperion, "getLedCount", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, ledCount));
	PyObject_SetAttrString(module, "ledCount", Py_BuildValue("i", ledCount));

	// add minimumWriteTime variable to the interpreter
	int latchTime = 0;
	QMetaObject::invokeMethod(_hyperion, "getLatchTime", Qt::BlockingQueuedConnection, Q_RETURN_ARG(int, latchTime));
	PyObject_SetAttrString(module, "latchTime", Py_BuildValue("i", latchTime));

	// add a args variable to the interpreter
	PyObject_SetAttrString(module, "args", EffectModule::json2python(_args));

	// decref the module
	Py_XDECREF(module);
}

void Effect::run()
{
	PythonProgram program(_name, _log);

	setModuleParameters();

	// Set the end time if applicable
	if (_timeout > 0)
	{
		_endTime = QDateTime::currentMSecsSinceEpoch() + _timeout;
	}

	// Run the effect script
	QFile file (_script);
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
