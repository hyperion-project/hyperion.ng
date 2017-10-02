// Python include
#include <Python.h>
#include <frameobject.h>

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

// effect engin eincludes
#include "Effect.h"
#include <utils/Logger.h>
#include <hyperion/Hyperion.h>

// Python method table
PyMethodDef Effect::effectMethods[] = {
	{"setColor"              , Effect::wrapSetColor              , METH_VARARGS, "Set a new color for the leds."},
	{"setImage"              , Effect::wrapSetImage              , METH_VARARGS, "Set a new image to process and determine new led colors."},
	{"abort"                 , Effect::wrapAbort                 , METH_NOARGS,  "Check if the effect should abort execution."},
	{"imageShow"             , Effect::wrapImageShow             , METH_VARARGS,  "set current effect image to hyperion core."},
	{"imageLinearGradient"   , Effect::wrapImageLinearGradient   , METH_VARARGS,  ""},
	{"imageConicalGradient"  , Effect::wrapImageConicalGradient  , METH_VARARGS,  ""},
	{"imageRadialGradient"   , Effect::wrapImageRadialGradient   , METH_VARARGS,  ""},
	{"imageSolidFill"        , Effect::wrapImageSolidFill        , METH_VARARGS,  ""},
	{"imageDrawLine"         , Effect::wrapImageDrawLine         , METH_VARARGS,  ""},
	{"imageDrawPoint"        , Effect::wrapImageDrawPoint        , METH_VARARGS,  ""},
	{"imageDrawRect"         , Effect::wrapImageDrawRect         , METH_VARARGS,  ""},
	{"imageDrawPolygon"      , Effect::wrapImageDrawPolygon      , METH_VARARGS,  ""},
	{"imageDrawPie"          , Effect::wrapImageDrawPie          , METH_VARARGS,  ""},
	{"imageSetPixel"         , Effect::wrapImageSetPixel         , METH_VARARGS, "set pixel color of image"},
	{"imageGetPixel"         , Effect::wrapImageGetPixel         , METH_VARARGS, "get pixel color of image"},
	{"imageSave"             , Effect::wrapImageSave             , METH_NOARGS,  "adds a new background image"},
	{"imageMinSize"          , Effect::wrapImageMinSize          , METH_VARARGS, "sets minimal dimension of background image"},
	{"imageWidth"            , Effect::wrapImageWidth            , METH_NOARGS,  "gets image width"},
	{"imageHeight"           , Effect::wrapImageHeight           , METH_NOARGS,  "gets image height"},
	{"imageCRotate"          , Effect::wrapImageCRotate          , METH_VARARGS, "rotate the coordinate system by given angle"},
	{"imageCOffset"          , Effect::wrapImageCOffset          , METH_VARARGS, "Add offset to the coordinate system"},
	{"imageCShear"           , Effect::wrapImageCShear           , METH_VARARGS, "Shear of coordinate system by the given horizontal/vertical axis"},
	{"imageResetT"           , Effect::wrapImageResetT           , METH_NOARGS,  "Resets all coords modifications (rotate,offset,shear)"},
	{NULL, NULL, 0, NULL}
};

#if PY_MAJOR_VERSION >= 3
// create the hyperion module
struct PyModuleDef Effect::moduleDef = {
	PyModuleDef_HEAD_INIT,
	"hyperion",            /* m_name */
	"Hyperion module",     /* m_doc */
	-1,                    /* m_size */
	Effect::effectMethods, /* m_methods */
	NULL,                  /* m_reload */
	NULL,                  /* m_traverse */
	NULL,                  /* m_clear */
	NULL,                  /* m_free */
};

PyObject* Effect::PyInit_hyperion()
{
	return PyModule_Create(&moduleDef);
}
#else
void Effect::PyInit_hyperion()
{
	Py_InitModule("hyperion", effectMethods);
}
#endif

void Effect::registerHyperionExtensionModule()
{
	PyImport_AppendInittab("hyperion", &PyInit_hyperion);
}

Effect::Effect(PyThreadState * mainThreadState, int priority, int timeout, const QString & script, const QString & name, const QJsonObject & args, const QString & origin, unsigned smoothCfg)
	: QThread()
	, _mainThreadState(mainThreadState)
	, _priority(priority)
	, _timeout(timeout)
	, _script(script)
	, _name(name)
	, _smoothCfg(smoothCfg)
	, _args(args)
	, _endTime(-1)
	, _interpreterThreadState(nullptr)
	, _abortRequested(false)
	, _imageProcessor(ImageProcessorFactory::getInstance().newImageProcessor())
	, _colors()
	, _origin(origin)
	, _imageSize(Hyperion::getInstance()->getLedGridSize())
	, _image(_imageSize,QImage::Format_ARGB32_Premultiplied)
{
	_colors.resize(_imageProcessor->getLedCount());
	_colors.fill(ColorRgb::BLACK);

	// disable the black border detector for effects
	_imageProcessor->enableBlackBorderDetector(false);

	// init effect image for image based effects, size is based on led layout
	_image.fill(Qt::black);
	_painter = new QPainter(&_image);

	// connect the finished signal
	connect(this, SIGNAL(finished()), this, SLOT(effectFinished()));
	Q_INIT_RESOURCE(EffectEngine);
}

Effect::~Effect()
{
	delete _painter;
	_imageStack.clear();
}

void Effect::run()
{
	// switch to the main thread state and acquire the GIL
	PyEval_RestoreThread(_mainThreadState);

	// Initialize a new thread state
	_interpreterThreadState = Py_NewInterpreter();

	// import the buildtin Hyperion module
	PyObject * module = PyImport_ImportModule("hyperion");

	// add a capsule containing 'this' to the module to be able to retrieve the effect from the callback function
	PyObject_SetAttrString(module, "__effectObj", PyCapsule_New(this, nullptr, nullptr));

	// add ledCount variable to the interpreter
	PyObject_SetAttrString(module, "ledCount", Py_BuildValue("i", _imageProcessor->getLedCount()));

	// add minimumWriteTime variable to the interpreter
	PyObject_SetAttrString(module, "latchTime", Py_BuildValue("i", Hyperion::getInstance()->getLatchTime()));

	// add a args variable to the interpreter
	PyObject_SetAttrString(module, "args", json2python(_args));

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
		Error(Logger::getInstance("EFFECTENGINE"), "Unable to open script file %s.", QSTRING_CSTR(_script));
	}
	file.close();

	if (!python_code.isEmpty())
	{
		PyRun_SimpleString(python_code.constData());
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

PyObject *Effect::json2python(const QJsonValue &jsonData) const
{
	switch (jsonData.type())
	{
		case QJsonValue::Null:
			return Py_BuildValue("");
		case QJsonValue::Undefined:
			return Py_BuildValue("");
		case QJsonValue::Double:
		{
			if (std::rint(jsonData.toDouble()) != jsonData.toDouble())
			{
				return Py_BuildValue("d", jsonData.toDouble());
			}
			return Py_BuildValue("i", jsonData.toInt());
		}
		case QJsonValue::Bool:
			return Py_BuildValue("i", jsonData.toBool() ? 1 : 0);
		case QJsonValue::String:
			return Py_BuildValue("s", jsonData.toString().toUtf8().constData());
		case QJsonValue::Object:
		{
			PyObject * dict= PyDict_New();
			QJsonObject objectData = jsonData.toObject();
			for (QJsonObject::iterator i = objectData.begin(); i != objectData.end(); ++i)
			{
				PyObject * obj = json2python(*i);
				PyDict_SetItemString(dict, i.key().toStdString().c_str(), obj);
				Py_XDECREF(obj);
			}
			return dict;
		}
		case QJsonValue::Array:
		{
			QJsonArray arrayData = jsonData.toArray();
			PyObject * list = PyList_New(arrayData.size());
			int index = 0;
			for (QJsonArray::iterator i = arrayData.begin(); i != arrayData.end(); ++i, ++index)
			{
				PyObject * obj = json2python(*i);
				Py_INCREF(obj);
				PyList_SetItem(list, index, obj);
				Py_XDECREF(obj);
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
	Effect * effect = getEffect();

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
			effect->_colors.fill(color);
			effect->setColors(effect->_priority, effect->_colors.toStdVector(), timeout, false, hyperion::COMP_EFFECT, effect->_origin, effect->_smoothCfg);
			return Py_BuildValue("");
		}
		return nullptr;
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
					effect->setColors(effect->_priority, effect->_colors.toStdVector(), timeout, false, hyperion::COMP_EFFECT, effect->_origin, effect->_smoothCfg);
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
	Effect * effect = getEffect();

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
				std::vector<ColorRgb> v = effect->_colors.toStdVector();
				effect->_imageProcessor->process(image, v);
				effect->setColors(effect->_priority, v, timeout, false, hyperion::COMP_EFFECT, effect->_origin, effect->_smoothCfg);
				return Py_BuildValue("");
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should be 3*width*height");
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
	Effect * effect = getEffect();

	// Test if the effect has reached it end time
	if (effect->_timeout > 0 && QDateTime::currentMSecsSinceEpoch() > effect->_endTime)
	{
		effect->_abortRequested = true;
	}

	return Py_BuildValue("i", effect->_abortRequested ? 1 : 0);
}


PyObject* Effect::wrapImageShow(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

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

	int argCount = PyTuple_Size(args);
	int imgId = -1;
	bool argsOk = (argCount == 0);
	if (argCount == 1 && PyArg_ParseTuple(args, "i", &imgId))
	{
		argsOk = true;
	}

	if ( ! argsOk || (imgId>-1 && imgId >= effect->_imageStack.size()))
	{
		return nullptr;
	}


	QImage * qimage = (imgId<0) ? &(effect->_image) : &(effect->_imageStack[imgId]);
	int width = qimage->width();
	int height = qimage->height();

	Image<ColorRgb> image(width, height);
	QByteArray binaryImage;

	for (int i = 0; i<height; ++i)
	{
		const QRgb * scanline = reinterpret_cast<const QRgb *>(qimage->scanLine(i));
		for (int j = 0; j< width; ++j)
		{
			binaryImage.append((char) qRed(scanline[j]));
			binaryImage.append((char) qGreen(scanline[j]));
			binaryImage.append((char) qBlue(scanline[j]));
		}
	}

	memcpy(image.memptr(), binaryImage.data(), binaryImage.size());
	std::vector<ColorRgb> v = effect->_colors.toStdVector();
	effect->_imageProcessor->process(image, v);
	effect->setColors(effect->_priority, v, timeout, false, hyperion::COMP_EFFECT, effect->_origin, effect->_smoothCfg);

	return Py_BuildValue("");
}

PyObject* Effect::wrapImageLinearGradient(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	PyObject * bytearray = nullptr;
	int startRX = 0;
	int startRY = 0;
	int startX = 0;
	int startY = 0;
	int endX, width = effect->_imageSize.width();
	int endY, height = effect->_imageSize.height();
	int spread = 0;

	bool argsOK = false;

	if ( argCount == 10 && PyArg_ParseTuple(args, "iiiiiiiiOi", &startRX, &startRY, &width, &height, &startX, &startY, &endX, &endY, &bytearray, &spread) )
	{
		argsOK = true;
	}
	if ( argCount == 6 && PyArg_ParseTuple(args, "iiiiOi", &startX, &startY, &endX, &endY, &bytearray, &spread) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		if (PyByteArray_Check(bytearray))
		{
			const int length = PyByteArray_Size(bytearray);
			const unsigned arrayItemLength = 5;
			if (length % arrayItemLength == 0)
			{
				QRect myQRect(startRX,startRY,width,height);
				QLinearGradient gradient(QPoint(startX,startY), QPoint(endX,endY));
				char * data = PyByteArray_AS_STRING(bytearray);

				for (int idx=0; idx<length; idx+=arrayItemLength)
				{
					gradient.setColorAt(
						((uint8_t)data[idx])/255.0,
						QColor(
							(uint8_t)(data[idx+1]),
							(uint8_t)(data[idx+2]),
							(uint8_t)(data[idx+3]),
							(uint8_t)(data[idx+4])
					));
				}

				gradient.setSpread(static_cast<QGradient::Spread>(spread));
				effect->_painter->fillRect(myQRect, gradient);

				return Py_BuildValue("");
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 5");
				return nullptr;
			}
		}
		else
		{
			PyErr_SetString(PyExc_RuntimeError, "No bytearray properly defined");
			return nullptr;
		}
	}
	return nullptr;
}

PyObject* Effect::wrapImageConicalGradient(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	PyObject * bytearray = nullptr;
	int centerX, centerY, angle;
	int startX = 0;
	int startY = 0;
	int width  = effect->_imageSize.width();
	int height = effect->_imageSize.height();

	bool argsOK = false;

	if ( argCount == 8 && PyArg_ParseTuple(args, "iiiiiiiO", &startX, &startY, &width, &height, &centerX, &centerY, &angle, &bytearray) )
	{
		argsOK = true;
	}
	if ( argCount == 4 && PyArg_ParseTuple(args, "iiiO", &centerX, &centerY, &angle, &bytearray) )
	{
		argsOK = true;
	}
	angle = qMax(qMin(angle,360),0);

	if (argsOK)
	{
		if (PyByteArray_Check(bytearray))
		{
			const int length = PyByteArray_Size(bytearray);
			const unsigned arrayItemLength = 5;
			if (length % arrayItemLength == 0)
			{
				QRect myQRect(startX,startY,width,height);
				QConicalGradient gradient(QPoint(centerX,centerY), angle );
				char * data = PyByteArray_AS_STRING(bytearray);

				for (int idx=0; idx<length; idx+=arrayItemLength)
				{
					gradient.setColorAt(
						((uint8_t)data[idx])/255.0,
						QColor(
							(uint8_t)(data[idx+1]),
							(uint8_t)(data[idx+2]),
							(uint8_t)(data[idx+3]),
							(uint8_t)(data[idx+4])
					));
				}

				effect->_painter->fillRect(myQRect, gradient);

				return Py_BuildValue("");
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 5");
				return nullptr;
			}
		}
		else
		{
			PyErr_SetString(PyExc_RuntimeError, "Argument 8 is not a bytearray");
			return nullptr;
		}
	}
	return nullptr;
}


PyObject* Effect::wrapImageRadialGradient(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	PyObject * bytearray = nullptr;
	int centerX, centerY, radius, focalX, focalY, focalRadius, spread;
	int startX = 0;
	int startY = 0;
	int width  = effect->_imageSize.width();
	int height = effect->_imageSize.height();

	bool argsOK = false;

	if ( argCount == 12 && PyArg_ParseTuple(args, "iiiiiiiiiiOi", &startX, &startY, &width, &height, &centerX, &centerY, &radius, &focalX, &focalY, &focalRadius, &bytearray, &spread) )
	{
		argsOK      = true;
	}
	if ( argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiOi", &startX, &startY, &width, &height, &centerX, &centerY, &radius, &bytearray, &spread) )
	{
		argsOK      = true;
		focalX      = centerX;
		focalY      = centerY;
		focalRadius = radius;
	}
	if ( argCount == 8 && PyArg_ParseTuple(args, "iiiiiiOi", &centerX, &centerY, &radius, &focalX, &focalY, &focalRadius, &bytearray, &spread) )
	{
		argsOK = true;
	}
	if ( argCount == 5 && PyArg_ParseTuple(args, "iiiOi", &centerX, &centerY, &radius, &bytearray, &spread) )
	{
		argsOK      = true;
		focalX      = centerX;
		focalY      = centerY;
		focalRadius = radius;
	}

	if (argsOK)
	{
		if (PyByteArray_Check(bytearray))
		{
			int length = PyByteArray_Size(bytearray);
			if (length % 4 == 0)
			{

				QRect myQRect(startX,startY,width,height);
				QRadialGradient gradient(QPoint(centerX,centerY), qMax(radius,0) );
				char * data = PyByteArray_AS_STRING(bytearray);

				for (int idx=0; idx<length; idx+=4)
				{
					gradient.setColorAt(
						((uint8_t)data[idx])/255.0,
						QColor(
							(uint8_t)(data[idx+1]),
							(uint8_t)(data[idx+2]),
							(uint8_t)(data[idx+3])
					));
				}

				gradient.setSpread(static_cast<QGradient::Spread>(spread));
				effect->_painter->fillRect(myQRect, gradient);

				return Py_BuildValue("");
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 4");
				return nullptr;
			}
		}
		else
		{
			PyErr_SetString(PyExc_RuntimeError, "Last argument is not a bytearray");
			return nullptr;
		}
	}
	return nullptr;
}

PyObject* Effect::wrapImageDrawPolygon(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();
	PyObject * bytearray = nullptr;

	int argCount = PyTuple_Size(args);
	int r, g, b;
	int a = 255;

	bool argsOK = false;

	if ( argCount == 5 && PyArg_ParseTuple(args, "Oiiii", &bytearray, &r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 4 && PyArg_ParseTuple(args, "Oiii", &bytearray, &r, &g, &b) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		if (PyByteArray_Check(bytearray))
		{
			int length = PyByteArray_Size(bytearray);
			if (length % 2 == 0)
			{
				QVector <QPoint> points;
				char * data = PyByteArray_AS_STRING(bytearray);

				for (int idx=0; idx<length; idx+=2)
				{
					points.append(QPoint((int)(data[idx]),(int)(data[idx+1])));
				}

				QPainter * painter = effect->_painter;
				QPen oldPen = painter->pen();
				QPen newPen(QColor(r,g,b,a));
				painter->setPen(newPen);
				painter->setBrush(QBrush(QColor(r,g,b,a), Qt::SolidPattern));
				painter->drawPolygon(points);
				painter->setPen(oldPen);
				return Py_BuildValue("");
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 2");
				return nullptr;
			}
		}
		else
		{
			PyErr_SetString(PyExc_RuntimeError, "Argument 1 is not a bytearray");
			return nullptr;
		}
	}
	return nullptr;
}

PyObject* Effect::wrapImageDrawPie(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();
	PyObject * bytearray = nullptr;

	QString brush;
	int argCount = PyTuple_Size(args);
	int radius, centerX, centerY;
	int startAngle = 0;
	int spanAngle = 360;
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;

	bool argsOK = false;

	if ( argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiii", &centerX, &centerY, &radius, &startAngle, &spanAngle, &r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &centerX, &centerY, &radius, &startAngle, &spanAngle, &r, &g, &b) )
	{
		argsOK = true;
	}
	if ( argCount == 7 && PyArg_ParseTuple(args, "iiiiisO", &centerX, &centerY, &radius, &startAngle, &spanAngle, &brush, &bytearray) )
	{
		argsOK = true;
	}
	if ( argCount == 5 && PyArg_ParseTuple(args, "iiisO", &centerX, &centerY, &radius, &brush, &bytearray) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter * painter = effect->_painter;
		startAngle = qMax(qMin(startAngle,360),0);
		spanAngle = qMax(qMin(spanAngle,360),-360);

		if( argCount == 7 || argCount == 5 )
		{
			a = 0;
			if (PyByteArray_Check(bytearray))
			{
				int length = PyByteArray_Size(bytearray);
				if (length % 5 == 0)
				{

						QConicalGradient gradient(QPoint(centerX,centerY), startAngle);


					char * data = PyByteArray_AS_STRING(bytearray);

					for (int idx=0; idx<length; idx+=5)
					{
						gradient.setColorAt(
							((uint8_t)data[idx])/255.0,
							QColor(
								(uint8_t)(data[idx+1]),
								(uint8_t)(data[idx+2]),
								(uint8_t)(data[idx+3]),
								(uint8_t)(data[idx+4])
						));
					}
					painter->setBrush(gradient);

					return Py_BuildValue("");
				}
				else
				{
					PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 5");
					return nullptr;
				}
			}
			else
			{
				PyErr_SetString(PyExc_RuntimeError, "Last argument is not a bytearray");
				return nullptr;
			}
		}
		else
		{
			painter->setBrush(QBrush(QColor(r,g,b,a), Qt::SolidPattern));
		}
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		painter->setPen(newPen);
		painter->drawPie(centerX - radius, centerY - radius, centerX + radius, centerY + radius, startAngle * 16, spanAngle * 16);
		painter->setPen(oldPen);
		return Py_BuildValue("");
	}
	return nullptr;
}

PyObject* Effect::wrapImageSolidFill(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int r, g, b;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int width  = effect->_imageSize.width();
	int height = effect->_imageSize.height();

	bool argsOK = false;

	if ( argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &startX, &startY, &width, &height, &r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 7 && PyArg_ParseTuple(args, "iiiiiii", &startX, &startY, &width, &height, &r, &g, &b) )
	{
		argsOK = true;
	}
	if ( argCount == 4 && PyArg_ParseTuple(args, "iiii",&r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 3 && PyArg_ParseTuple(args, "iii",&r, &g, &b) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QRect myQRect(startX,startY,width,height);
		effect->_painter->fillRect(myQRect, QColor(r,g,b,a));
		return Py_BuildValue("");
	}
	return nullptr;
}


PyObject* Effect::wrapImageDrawLine(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int r, g, b;
	int a      = 255;
	int startX = 0;
	int startY = 0;
	int thick  = 1;
	int endX   = effect->_imageSize.width();
	int endY   = effect->_imageSize.height();

	bool argsOK = false;

	if ( argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiii", &startX, &startY, &endX, &endY, &thick, &r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &startX, &startY, &endX, &endY, &thick, &r, &g, &b) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter * painter = effect->_painter;
		QRect myQRect(startX, startY, endX, endY);
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawLine(startX, startY, endX, endY);
		painter->setPen(oldPen);

		return Py_BuildValue("");
	}
	return nullptr;
}

PyObject* Effect::wrapImageDrawPoint(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int r, g, b, x, y;
	int a      = 255;
	int thick  = 1;

	bool argsOK = false;

	if ( argCount == 7 && PyArg_ParseTuple(args, "iiiiiii", &x, &y, &thick, &r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 6 && PyArg_ParseTuple(args, "iiiiii", &x, &y, &thick, &r, &g, &b) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter * painter = effect->_painter;
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawPoint(x, y);
		painter->setPen(oldPen);

		return Py_BuildValue("");
	}
	return nullptr;
}

PyObject* Effect::wrapImageDrawRect(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int r, g, b;
	int a      = 255;
	int startX = 0;
	int startY = 0;
	int thick  = 1;
	int width   = effect->_imageSize.width();
	int height  = effect->_imageSize.height();

	bool argsOK = false;

	if ( argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiii", &startX, &startY, &width, &height, &thick, &r, &g, &b, &a) )
	{
		argsOK = true;
	}
	if ( argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &startX, &startY, &width, &height, &thick, &r, &g, &b) )
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter * painter = effect->_painter;
		QRect myQRect(startX,startY,width,height);
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawRect(startX, startY, width, height);
		painter->setPen(oldPen);

		return Py_BuildValue("");
	}
	return nullptr;
}


PyObject* Effect::wrapImageSetPixel(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int r, g, b, x, y;

	if ( argCount == 5 && PyArg_ParseTuple(args, "iiiii", &x, &y, &r, &g, &b ) )
	{
		effect->_image.setPixel(x,y,qRgb(r,g,b));
		return Py_BuildValue("");
	}

	return nullptr;
}


PyObject* Effect::wrapImageGetPixel(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int x, y;

	if ( argCount == 2 && PyArg_ParseTuple(args, "ii", &x, &y) )
	{
		QRgb rgb = effect->_image.pixel(x,y);
		return Py_BuildValue("iii",qRed(rgb),qGreen(rgb),qBlue(rgb));
	}
	return nullptr;
}

PyObject* Effect::wrapImageSave(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();
	QImage img(effect->_image.copy());
	effect->_imageStack.append(img);

	return Py_BuildValue("i", effect->_imageStack.size()-1);
}

PyObject* Effect::wrapImageMinSize(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int w, h;
	int width   = effect->_imageSize.width();
	int height  = effect->_imageSize.height();

	if ( argCount == 2 && PyArg_ParseTuple(args, "ii", &w, &h) )
	{
		if (width<w || height<h)
		{
			delete effect->_painter;

			effect->_image = effect->_image.scaled(qMax(width,w),qMax(height,h), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
			effect->_imageSize = effect->_image.size();
			effect->_painter = new QPainter(&(effect->_image));
		}
		return Py_BuildValue("ii", effect->_image.width(), effect->_image.height());
	}
	return nullptr;
}

PyObject* Effect::wrapImageWidth(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();
	return Py_BuildValue("i", effect->_imageSize.width());
}

PyObject* Effect::wrapImageHeight(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();
	return Py_BuildValue("i", effect->_imageSize.height());
}

PyObject* Effect::wrapImageCRotate(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int argCount = PyTuple_Size(args);
	int angle;

	if ( argCount == 1 && PyArg_ParseTuple(args, "i", &angle ) )
	{
		angle = qMax(qMin(angle,360),0);
		effect->_painter->rotate(angle);
		return Py_BuildValue("");
	}
	return nullptr;
}

PyObject* Effect::wrapImageCOffset(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int offsetX = 0;
	int offsetY = 0;
	int argCount = PyTuple_Size(args);

	if ( argCount == 2 )
	{
		PyArg_ParseTuple(args, "ii", &offsetX, &offsetY );
	}

	effect->_painter->translate(QPoint(offsetX,offsetY));
	return Py_BuildValue("");
}

PyObject* Effect::wrapImageCShear(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	int sh,sv;
	int argCount = PyTuple_Size(args);

	if ( argCount == 2 && PyArg_ParseTuple(args, "ii", &sh, &sv ))
	{
		effect->_painter->shear(sh,sv);
		return Py_BuildValue("");
	}
	return nullptr;
}

PyObject* Effect::wrapImageResetT(PyObject *self, PyObject *args)
{
	Effect * effect = getEffect();

	effect->_painter->resetTransform();
	return Py_BuildValue("");
}

Effect * Effect::getEffect()
{
	// extract the module from the runtime
	PyObject * module = PyObject_GetAttrString(PyImport_AddModule("__main__"), "hyperion");

	if (!PyModule_Check(module))
	{
		// something is wrong
		Py_XDECREF(module);
		Error(Logger::getInstance("EFFECTENGINE"), "Unable to retrieve the effect object from the Python runtime");
		return nullptr;
	}

	// retrieve the capsule with the effect
	PyObject * effectCapsule = PyObject_GetAttrString(module, "__effectObj");
	Py_XDECREF(module);

	if (!PyCapsule_CheckExact(effectCapsule))
	{
		// something is wrong
		Py_XDECREF(effectCapsule);
		Error(Logger::getInstance("EFFECTENGINE"), "Unable to retrieve the effect object from the Python runtime");
		return nullptr;
	}

	// Get the effect from the capsule
	Effect * effect = reinterpret_cast<Effect *>(PyCapsule_GetPointer(effectCapsule, nullptr));
	Py_XDECREF(effectCapsule);
	return effect;
}
