#include <cmath>

#include <effectengine/Effect.h>
#include <effectengine/EffectModule.h>

// hyperion
#include <hyperion/Hyperion.h>
#include <utils/Logger.h>

// qt
#include <QJsonArray>
#include <QDateTime>
#include <QImageReader>
#include <QBuffer>
#include <QUrl>
#include <QNetworkReply>
#include <QNetworkAccessManager>
#include <QEventLoop>

// Define a struct for per-interpreter state
using hyperion_module_state = struct {
};

// Macro to access the module state
#define GET_HYPERION_STATE(hyperionModule) ((hyperion_module_state*)PyModule_GetState(hyperionModule))

// Get the effect from the capsule
#define getEffect() static_cast<Effect*>((Effect*)PyCapsule_Import("hyperion.__effectObj", 0))

// Module execution function for multi-phase init
static int hyperion_exec(PyObject* hyperionModule) {
	// Initialize per-interpreter state
	hyperion_module_state const* state = GET_HYPERION_STATE(hyperionModule);
	if (state == nullptr)
	{
		return -1;
	}
	return 0;
}

// Module deallocation function to clean up per-interpreter state
static void hyperion_free(void* /* hyperionModule */) // NOSONAR - signature mandated by Python C API freefunc typedef
{
	// No specific cleanup required in this example
}

static PyModuleDef_Slot hyperion_slots[] = { // NOSONAR - C-style array required by PyModuleDef_Slot Python C API
	{Py_mod_exec, reinterpret_cast<void*>(hyperion_exec)}, // NOSONAR
#if (PY_VERSION_HEX >= 0x030C0000)
	{Py_mod_multiple_interpreters, Py_MOD_PER_INTERPRETER_GIL_SUPPORTED},
#endif
	{0, nullptr}
};

// Module definition with multi-phase and per-interpreter state
static struct PyModuleDef hyperion_module = { // NOSONAR - m_slots is assigned in PyInit_hyperion; struct must be mutable
	PyModuleDef_HEAD_INIT,
	"hyperion",                    // Module name
	"Hyperion module",             // Module docstring
	sizeof(hyperion_module_state), // Size of per-interpreter state
	EffectModule::effectMethods,   // Methods array
	nullptr,                       // Slots array (will be added in PyInit_hyperion)
	nullptr,                       // Traverse function (optional)
	nullptr,                       // Clear function (optional)
	hyperion_free                  // Free function
};

// initialize function for the hyperion module
PyMODINIT_FUNC PyInit_hyperion(void)
{

	// assign slots to the module definition
	hyperion_module.m_slots = hyperion_slots;

	// return a new module definition instance
	return PyModuleDef_Init(&hyperion_module);
}

void EffectModule::registerHyperionExtensionModule()
{
	PyImport_AppendInittab("hyperion", &PyInit_hyperion);
}

static PyObject* json2pythonArray(const QJsonValue& jsonData)
{
	QJsonArray arrayData = jsonData.toArray();
	PyObject* list = PyList_New(arrayData.size());
	if (!list)
	{
		return nullptr; // Allocation failed
	}
	int index = 0;
	for (auto i = arrayData.begin(); i != arrayData.end(); ++i, ++index)
	{
		PyObject* obj = EffectModule::json2python(*i);
		if (!obj)
		{
			Py_DECREF(list);
			return nullptr; // Error occurred, return null
		}
		// PyList_SetItem unconditionally steals the reference to obj, even on failure
		if (PyList_SetItem(list, index, obj) != 0)
		{
			Py_DECREF(list);
			return nullptr; // Failed to insert item
		}
	}
	return list;
}

static PyObject* json2pythonObject(const QJsonValue& jsonData)
{
	// Python's dict
	QJsonObject jsonObject = jsonData.toObject();
	PyObject* pyDict = PyDict_New();
	if (!pyDict)
	{
		return nullptr; // Allocation failed
	}
	for (auto it = jsonObject.begin(); it != jsonObject.end(); ++it)
	{
		// Convert key
		PyObject* pyKey = PyUnicode_FromString(it.key().toUtf8().constData());
		if (!pyKey)
		{
			Py_DECREF(pyDict);
			return nullptr; // Error occurred, return null
		}
		// Convert value
		PyObject* pyValue = EffectModule::json2python(it.value());
		if (!pyValue)
		{
			Py_DECREF(pyKey);
			Py_DECREF(pyDict);
			return nullptr; // Error occurred, return null
		}
		// Add to dictionary with error check
		if (PyDict_SetItem(pyDict, pyKey, pyValue) < 0)
		{
			Py_DECREF(pyKey);
			Py_DECREF(pyValue);
			Py_DECREF(pyDict);
			return nullptr; // insertion failed
		}
		Py_DECREF(pyKey);
		Py_DECREF(pyValue);
	}
	return pyDict;
}

PyObject* EffectModule::json2python(const QJsonValue& jsonData)
{
	switch (jsonData.type())
	{
		case QJsonValue::Null:
			Py_RETURN_NONE;

		case QJsonValue::Undefined:
			Py_RETURN_NOTIMPLEMENTED;

		case QJsonValue::Double:
		{
			double value = jsonData.toDouble();
			if (value == static_cast<int>(value))  // If no fractional part, value is equal to its integer representation
			{
				return Py_BuildValue("i", static_cast<int>(value));
			}
			return Py_BuildValue("d", value);
		}

		case QJsonValue::Bool:
			return PyBool_FromLong(jsonData.toBool() ? 1 : 0);

		case QJsonValue::String:
			return PyUnicode_FromString(jsonData.toString().toUtf8().constData());

		case QJsonValue::Array:
			return json2pythonArray(jsonData);

		case QJsonValue::Object:
			return json2pythonObject(jsonData);

	default:
		PyErr_SetString(PyExc_TypeError, "Unsupported QJsonValue type.");
		return nullptr;
	}
}

// Python method table
PyMethodDef EffectModule::effectMethods[] = { // NOSONAR - C-style array required by PyMethodDef Python C API
	{"setColor"              , EffectModule::wrapSetColor              , METH_VARARGS, "Set a new color for the leds."},
	{"setImage"              , EffectModule::wrapSetImage              , METH_VARARGS, "Set a new image to process and determine new led colors."},
	{"getImage"              , EffectModule::wrapGetImage              , METH_VARARGS, "get image data from file."},
	{"abort"                 , EffectModule::wrapAbort                 , METH_NOARGS,  "Check if the effect should abort execution."},
	{"imageShow"             , EffectModule::wrapImageShow             , METH_VARARGS,  "set current effect image to hyperion core."},
	{"imageLinearGradient"   , EffectModule::wrapImageLinearGradient   , METH_VARARGS,  ""},
	{"imageConicalGradient"  , EffectModule::wrapImageConicalGradient  , METH_VARARGS,  ""},
	{"imageRadialGradient"   , EffectModule::wrapImageRadialGradient   , METH_VARARGS,  ""},
	{"imageSolidFill"        , EffectModule::wrapImageSolidFill        , METH_VARARGS,  ""},
	{"imageDrawLine"         , EffectModule::wrapImageDrawLine         , METH_VARARGS,  ""},
	{"imageDrawPoint"        , EffectModule::wrapImageDrawPoint        , METH_VARARGS,  ""},
	{"imageDrawRect"         , EffectModule::wrapImageDrawRect         , METH_VARARGS,  ""},
	{"imageDrawPolygon"      , EffectModule::wrapImageDrawPolygon      , METH_VARARGS,  ""},
	{"imageDrawPie"          , EffectModule::wrapImageDrawPie          , METH_VARARGS,  ""},
	{"imageSetPixel"         , EffectModule::wrapImageSetPixel         , METH_VARARGS, "set pixel color of image"},
	{"imageGetPixel"         , EffectModule::wrapImageGetPixel         , METH_VARARGS, "get pixel color of image"},
	{"imageSave"             , EffectModule::wrapImageSave             , METH_NOARGS,  "adds a new background image"},
	{"imageMinSize"          , EffectModule::wrapImageMinSize          , METH_VARARGS, "sets minimal dimension of background image"},
	{"imageWidth"            , EffectModule::wrapImageWidth            , METH_NOARGS,  "gets image width"},
	{"imageHeight"           , EffectModule::wrapImageHeight           , METH_NOARGS,  "gets image height"},
	{"imageCRotate"          , EffectModule::wrapImageCRotate          , METH_VARARGS, "rotate the coordinate system by given angle"},
	{"imageCOffset"          , EffectModule::wrapImageCOffset          , METH_VARARGS, "Add offset to the coordinate system"},
	{"imageCShear"           , EffectModule::wrapImageCShear           , METH_VARARGS, "Shear of coordinate system by the given horizontal/vertical axis"},
	{"imageResetT"           , EffectModule::wrapImageResetT           , METH_NOARGS,  "Resets all coords modifications (rotate,offset,shear)"},
	{"lowestUpdateInterval"  , EffectModule::wrapLowestUpdateInterval  , METH_NOARGS,  "Gets the lowest permissible interval time in seconds"},
	{"imageStackClear"       , EffectModule::wrapImageStackClear       , METH_NOARGS,  "Clears the internal saved image stack"},
	{nullptr, nullptr, 0, nullptr}
};

PyObject* EffectModule::wrapSetColor(PyObject* /*self*/, PyObject* args)
{
	// check the number of arguments
	Py_ssize_t argCount = PyTuple_Size(args);
	if (argCount == 3)
	{
		// three separate arguments for red, green, and blue
		ColorRgb color;
		if (PyArg_ParseTuple(args, "bbb", &color.red, &color.green, &color.blue))
		{
			getEffect()->_colors.fill(color);
			QVector<ColorRgb> _cQV = getEffect()->_colors;
			emit getEffect()->setInput(getEffect()->_priority, QVector<ColorRgb>(_cQV.begin(), _cQV.end()), getEffect()->getRemaining(), false);
			Py_RETURN_NONE;
		}
		return nullptr;
	}
	else if (argCount == 1)
	{
		// bytearray of values
		PyObject* bytearray = nullptr;
		if (!PyArg_ParseTuple(args, "O", &bytearray))
		{
			return nullptr;
		}
		if (!PyByteArray_Check(bytearray))
		{
			PyErr_SetString(PyExc_RuntimeError, "Argument is not a bytearray");
			return nullptr;
		}
		size_t length = PyByteArray_Size(bytearray);
		if (length != 3 * static_cast<size_t>(getEffect()->_hyperionWeak.toStrongRef()->getLedCount()))
		{
			PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should be 3*ledCount");
			return nullptr;
		}
		const char* data = PyByteArray_AS_STRING(bytearray);
		memcpy(getEffect()->_colors.data(), data, length);
		QVector<ColorRgb> _cQV = getEffect()->_colors;
		emit getEffect()->setInput(getEffect()->_priority, QVector<ColorRgb>(_cQV.begin(), _cQV.end()), getEffect()->getRemaining(), false);
		Py_RETURN_NONE;
	}
	else
	{
		PyErr_SetString(PyExc_RuntimeError, "Function expect 1 or 3 arguments");
		return nullptr;
	}
}

PyObject* EffectModule::wrapSetImage(PyObject* /*self*/, PyObject* args)
{
	// bytearray of values
	int width = 0;
	int height = 0;
	PyObject* bytearray = nullptr;
	if (PyArg_ParseTuple(args, "iiO", &width, &height, &bytearray))
	{
		if (PyByteArray_Check(bytearray))
		{
			Py_ssize_t length = PyByteArray_Size(bytearray);
			if (length == 3 * width * height)
			{
				Image<ColorRgb> image(width, height);
				const char* data = PyByteArray_AS_STRING(bytearray);
				memcpy(image.memptr(), data, length);
				emit getEffect()->setInputImage(getEffect()->_priority, image, getEffect()->getRemaining(), false);
				Py_RETURN_NONE;
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

static QByteArray imageToRGB(const QImage& qimage, int width, int height, bool grayscale)
{
	qsizetype size = qsizetype(width) * qsizetype(height) * 3;
	QByteArray binaryImage(size, Qt::Uninitialized);
	char* dest = binaryImage.data();
	for (int i = 0; i < height; ++i)
	{
		const auto* scanline = reinterpret_cast<const QRgb*>(qimage.scanLine(i)); // NOSONAR - idiomatic Qt pattern for typed scanline access
		for (int j = 0; j < width; ++j)
		{
			QRgb pixel = scanline[j];
			*dest++ = static_cast<char>(!grayscale ? qRed(pixel) : qGray(pixel));
			*dest++ = static_cast<char>(!grayscale ? qGreen(pixel) : qGray(pixel));
			*dest++ = static_cast<char>(!grayscale ? qBlue(pixel) : qGray(pixel));
		}
	}
	return binaryImage;
}

struct ImageCropParams
{
	int cropLeft   = 0;
	int cropTop    = 0;
	int cropRight  = 0;
	int cropBottom = 0;
};

static bool setupImageSource(PyObject* args, const QString& imageData, QBuffer& buffer, QImageReader& reader, ImageCropParams& crop, bool& grayscale)
{
	char* source = nullptr;

	if (imageData.isEmpty())
	{
		qCDebug(effect) << "setupImageSource: no embedded imageData, parsing source from args";
		Q_INIT_RESOURCE(EffectEngine);

		int grayscaleInt = 0;
		if (!PyArg_ParseTuple(args, "s|iiiip", &source, &crop.cropLeft, &crop.cropTop, &crop.cropRight, &crop.cropBottom, &grayscaleInt))
		{
			PyErr_SetString(PyExc_TypeError, "String required");
			return false;
		}
		grayscale = (grayscaleInt != 0);

		const auto url = QUrl(source);
		if (url.isValid() && !url.scheme().isEmpty() && url.scheme() != "file")
		{
			qCDebug(effect) << "setupImageSource: fetching via network -" << url.toString();
			QNetworkAccessManager networkManager;
			QScopedPointer<QNetworkReply> networkReply(networkManager.get(QNetworkRequest(url)));

			QEventLoop eventLoop;
			QObject::connect(networkReply.get(), &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
			eventLoop.exec();

			if (networkReply->error() == QNetworkReply::NoError)
			{
				buffer.setData(networkReply->readAll());
				buffer.open(QBuffer::ReadOnly);
				reader.setDecideFormatFromContent(true);
				reader.setDevice(&buffer);
				qCDebug(effect) << "setupImageSource: network fetch OK, buffer size =" << buffer.size();
			}
			else
			{
				qCDebug(effect) << "setupImageSource: network error -" << networkReply->errorString();
			}
			// QScopedPointer automatically deletes networkReply here
		}
		else
		{
			QString file = QString::fromUtf8(source);

			if (file.mid(0, 1) == ":")
				file = ":/effects/" + file.mid(1);

			qCDebug(effect) << "setupImageSource: loading from file -" << file;
			reader.setDecideFormatFromContent(true);
			reader.setFileName(file);
		}
	}
	else
	{
		qCDebug(effect) << "setupImageSource: embedded imageData present, size =" << imageData.size();
		int grayscaleInt = 0;
		if (!PyArg_ParseTuple(args, "|siiiip", &source, &crop.cropLeft, &crop.cropTop, &crop.cropRight, &crop.cropBottom, &grayscaleInt))
		{
			return false;
		}
		grayscale = (grayscaleInt != 0);
		const QByteArray decoded = QByteArray::fromBase64(imageData.toUtf8());
		buffer.setData(decoded);
		buffer.open(QBuffer::ReadOnly);
		reader.setDecideFormatFromContent(true);
		reader.setDevice(&buffer);
	}
	return true;
}

PyObject* EffectModule::wrapGetImage(PyObject* /*self*/, PyObject* args)
{
	QBuffer buffer;
	QImageReader reader;
	ImageCropParams crop;
	bool grayscale = false;

	if (!setupImageSource(args, getEffect()->_imageData, buffer, reader, crop, grayscale))
	{
		return nullptr;
	}

	if (!reader.canRead())
	{
		qCDebug(effect) << "wrapGetImage: reader cannot read -" << reader.errorString();
		PyErr_SetString(PyExc_TypeError, reader.errorString().toUtf8().constData());
		return nullptr;
	}

	// imageCount() returns 0 for non-animated formats (JPEG, PNG, …); treat as 1 frame
	const int rawImageCount = reader.imageCount();
	const int imageCount = qMax(rawImageCount, 1);
	const bool isAnimated = (rawImageCount > 0);
	qCDebug(effect) << "wrapGetImage: format =" << reader.format()
	                         << "| imageCount() =" << rawImageCount
	                         << "| treating as" << imageCount << "frame(s)"
	                         << "| animated =" << isAnimated;

	PyObject* result = PyList_New(imageCount);
	if (!result) return nullptr;

	for (int imageNumber = 0; imageNumber < imageCount; ++imageNumber)
	{
		if (isAnimated)
		{
			reader.jumpToImage(imageNumber);
			if (!reader.canRead())
			{
				qCDebug(effect) << "wrapGetImage: frame" << imageNumber << "not readable -" << reader.errorString();
				Py_DECREF(result);
				PyErr_SetString(PyExc_TypeError, reader.errorString().toUtf8().constData());
				return nullptr;
			}
		}

		QImage qimage = reader.read();
		int width = qimage.width();
		int height = qimage.height();
		qCDebug(effect) << "wrapGetImage: frame" << imageNumber << "decoded:" << width << "x" << height
		                         << "| image isNull =" << qimage.isNull()
		                         << "| grayscale =" << grayscale;

		if (crop.cropLeft > 0 || crop.cropTop > 0 || crop.cropRight > 0 || crop.cropBottom > 0)
		{
			if (crop.cropLeft + crop.cropRight >= width || crop.cropTop + crop.cropBottom >= height)
			{
				Py_DECREF(result);
				QString errorStr = QString("Rejecting invalid crop values: left: %1, right: %2, top: %3, bottom: %4, higher than height/width %5/%6").arg(crop.cropLeft).arg(crop.cropRight).arg(crop.cropTop).arg(crop.cropBottom).arg(height).arg(width);
				PyErr_SetString(PyExc_RuntimeError, qPrintable(errorStr));
				return nullptr;
			}

			qimage = qimage.copy(crop.cropLeft, crop.cropTop, width - crop.cropLeft - crop.cropRight, height - crop.cropTop - crop.cropBottom);
			width = qimage.width();
			height = qimage.height();
		}

		QByteArray binaryImage = imageToRGB(qimage, width, height, grayscale);

		PyObject* pyBytes = PyByteArray_FromStringAndSize(binaryImage.constData(), binaryImage.size());
		if (!pyBytes)
		{
			Py_DECREF(result);
			return nullptr;
		}

		// Use format unit 'N' to pass ownership of pyBytes to the dict without needing DECREF here
		PyObject* dict = Py_BuildValue("{s:i,s:i,s:N}", "imageWidth", width, "imageHeight", height, "imageData", pyBytes);

		if (!dict)
		{
			Py_DECREF(result);
			return nullptr;
		}

		// Replace macro with function to allow error check
		if (PyList_SetItem(result, imageNumber, dict) != 0)
		{
			Py_DECREF(result);
			return nullptr; // failed to insert
		}
	}

	return result;
}

PyObject* EffectModule::wrapAbort(PyObject* /*self*/, PyObject*)
{
	return Py_BuildValue("i", getEffect()->isInterruptionRequested() ? 1 : 0);
}


PyObject* EffectModule::wrapImageShow(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int imgId = -1;
	bool argsOk = (argCount == 0);
	if (argCount == 1 && PyArg_ParseTuple(args, "i", &imgId))
	{
		argsOk = true;
	}

	if (!argsOk)
	{
		if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
		return nullptr;
	}

	if (imgId > -1 && imgId >= getEffect()->_imageStack.size())
	{
		PyErr_SetString(PyExc_IndexError, "Image id out of range.");
		return nullptr;
	}


	QImage* qimage = (imgId < 0) ? &(getEffect()->_image) : &(getEffect()->_imageStack[imgId]);
	int width = qimage->width();
	int height = qimage->height();

	Image<ColorRgb> image(width, height);
	QByteArray binaryImage;

	for (int i = 0; i < height; ++i)
	{
		const auto* scanline = reinterpret_cast<const QRgb*>(qimage->scanLine(i)); // NOSONAR - idiomatic Qt pattern for typed scanline access
		for (int j = 0; j < width; ++j)
		{
			binaryImage.append((char)qRed(scanline[j]));
			binaryImage.append((char)qGreen(scanline[j]));
			binaryImage.append((char)qBlue(scanline[j]));
		}
	}

	memcpy(image.memptr(), binaryImage.data(), binaryImage.size());
	emit getEffect()->setInputImage(getEffect()->_priority, image, getEffect()->getRemaining(), false);

	return Py_BuildValue("");
}

PyObject* EffectModule::wrapImageLinearGradient(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	PyObject* bytearray = nullptr;
	int startRX = 0;
	int startRY = 0;
	int startX = 0;
	int startY = 0;
	int width = getEffect()->_imageSize.width();
	int endX{ width };
	int height = getEffect()->_imageSize.height();
	int endY{ height };
	int spread = 0;

	bool argsOK = false;

	if (argCount == 10 && PyArg_ParseTuple(args, "iiiiiiiiOi", &startRX, &startRY, &width, &height, &startX, &startY, &endX, &endY, &bytearray, &spread))
	{
		argsOK = true;
	}
	if (argCount == 6 && PyArg_ParseTuple(args, "iiiiOi", &startX, &startY, &endX, &endY, &bytearray, &spread))
	{
		argsOK = true;
	}

	if (!argsOK)
	{
		if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
		return nullptr;
	}

	if (!PyByteArray_Check(bytearray))
	{
		PyErr_SetString(PyExc_RuntimeError, "No bytearray properly defined");
		return nullptr;
	}

	const Py_ssize_t length = PyByteArray_Size(bytearray);
	const unsigned arrayItemLength = 5;
	if (length % arrayItemLength != 0)
	{
		PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 5");
		return nullptr;
	}

	QRect myQRect(startRX, startRY, width, height);
	QLinearGradient gradient(QPoint(startX, startY), QPoint(endX, endY));
	const char* data = PyByteArray_AS_STRING(bytearray);

	for (int idx = 0; idx < length; idx += arrayItemLength)
	{
		gradient.setColorAt(
			((uint8_t)data[idx]) / 255.0,
			QColor(
				(uint8_t)(data[idx + 1]),
				(uint8_t)(data[idx + 2]),
				(uint8_t)(data[idx + 3]),
				(uint8_t)(data[idx + 4])
			));
	}

	gradient.setSpread(static_cast<QGradient::Spread>(spread));
	getEffect()->_painter->fillRect(myQRect, gradient);

	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapImageConicalGradient(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	PyObject* bytearray = nullptr;
	int centerX = 0;
	int centerY = 0;
	int angle = 0;
	int startX = 0;
	int startY = 0;
	int width = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

	bool argsOK = false;

	if (argCount == 8 && PyArg_ParseTuple(args, "iiiiiiiO", &startX, &startY, &width, &height, &centerX, &centerY, &angle, &bytearray))
	{
		argsOK = true;
	}
	if (argCount == 4 && PyArg_ParseTuple(args, "iiiO", &centerX, &centerY, &angle, &bytearray))
	{
		argsOK = true;
	}
	angle = qMax(qMin(angle, 360), 0);

	if (!argsOK)
	{
		if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
		return nullptr;
	}

	if (!PyByteArray_Check(bytearray))
	{
		PyErr_SetString(PyExc_RuntimeError, "Argument 8 is not a bytearray");
		return nullptr;
	}

	const Py_ssize_t length = PyByteArray_Size(bytearray);
	const unsigned arrayItemLength = 5;
	if (length % arrayItemLength != 0)
	{
		PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 5");
		return nullptr;
	}

	QRect myQRect(startX, startY, width, height);
	QConicalGradient gradient(QPoint(centerX, centerY), angle);
	const char* data = PyByteArray_AS_STRING(bytearray);

	for (int idx = 0; idx < length; idx += arrayItemLength)
	{
		gradient.setColorAt(
			((uint8_t)data[idx]) / 255.0,
			QColor(
				(uint8_t)(data[idx + 1]),
				(uint8_t)(data[idx + 2]),
				(uint8_t)(data[idx + 3]),
				(uint8_t)(data[idx + 4])
			));
	}

	getEffect()->_painter->fillRect(myQRect, gradient);

	Py_RETURN_NONE;
}


PyObject* EffectModule::wrapImageRadialGradient(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	PyObject* bytearray = nullptr;
	int centerX = 0;
	int centerY = 0;
	int radius = 0;
	int focalX = 0;
	int focalY = 0;
	int focalRadius = 0;
	int spread = 0;
	int startX = 0;
	int startY = 0;
	int width = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

	bool argsOK = false;

	if (argCount == 12 && PyArg_ParseTuple(args, "iiiiiiiiiiOi", &startX, &startY, &width, &height, &centerX, &centerY, &radius, &focalX, &focalY, &focalRadius, &bytearray, &spread))
	{
		argsOK = true;
	}
	if (argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiOi", &startX, &startY, &width, &height, &centerX, &centerY, &radius, &bytearray, &spread))
	{
		argsOK = true;
		focalX = centerX;
		focalY = centerY;
		focalRadius = radius;
	}
	if (argCount == 8 && PyArg_ParseTuple(args, "iiiiiiOi", &centerX, &centerY, &radius, &focalX, &focalY, &focalRadius, &bytearray, &spread))
	{
		argsOK = true;
	}
	if (argCount == 5 && PyArg_ParseTuple(args, "iiiOi", &centerX, &centerY, &radius, &bytearray, &spread))
	{
		argsOK = true;
		focalX = centerX;
		focalY = centerY;
		focalRadius = radius;
	}

	if (!argsOK)
	{
		if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
		return nullptr;
	}

	if (!PyByteArray_Check(bytearray))
	{
		PyErr_SetString(PyExc_RuntimeError, "Last argument is not a bytearray");
		return nullptr;
	}

	Py_ssize_t length = PyByteArray_Size(bytearray);
	if (length % 4 != 0)
	{
		PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 4");
		return nullptr;
	}

	QRect myQRect(startX, startY, width, height);
	QRadialGradient gradient(QPoint(centerX, centerY), qMax(radius, 0));
	const char* data = PyByteArray_AS_STRING(bytearray);

	for (int idx = 0; idx < length; idx += 4)
	{
		gradient.setColorAt(
			((uint8_t)data[idx]) / 255.0,
			QColor(
				(uint8_t)(data[idx + 1]),
				(uint8_t)(data[idx + 2]),
				(uint8_t)(data[idx + 3])
			));
	}

	gradient.setSpread(static_cast<QGradient::Spread>(spread));
	getEffect()->_painter->fillRect(myQRect, gradient);

	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapImageDrawPolygon(PyObject* /*self*/, PyObject* args)
{
	PyObject* bytearray = nullptr;

	Py_ssize_t argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;

	bool argsOK = false;

	if (argCount == 5 && PyArg_ParseTuple(args, "Oiiii", &bytearray, &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 4 && PyArg_ParseTuple(args, "Oiii", &bytearray, &r, &g, &b))
	{
		argsOK = true;
	}

	if (!argsOK)
	{
		if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
		return nullptr;
	}

	if (!PyByteArray_Check(bytearray))
	{
		PyErr_SetString(PyExc_RuntimeError, "Argument 1 is not a bytearray");
		return nullptr;
	}

	Py_ssize_t length = PyByteArray_Size(bytearray);
	if (length % 2 != 0)
	{
		PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 2");
		return nullptr;
	}

	QVector<QPoint> points;
	const char* data = PyByteArray_AS_STRING(bytearray);

	for (int idx = 0; idx < length; idx += 2)
	{
		points.append(QPoint((int)(data[idx]), (int)(data[idx + 1])));
	}

	QPainter* painter = getEffect()->_painter.get();
	QPen oldPen = painter->pen();
	QPen newPen(QColor(r, g, b, a));
	painter->setPen(newPen);
	painter->setBrush(QBrush(QColor(r, g, b, a), Qt::SolidPattern));
	painter->drawPolygon(points);
	painter->setPen(oldPen);
	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapImageDrawPie(PyObject* /*self*/, PyObject* args)
{
	PyObject* bytearray = nullptr;

	QString brush;
	Py_ssize_t argCount = PyTuple_Size(args);
	int radius = 0;
	int centerX = 0;
	int centerY = 0;
	int startAngle = 0;
	int spanAngle = 360;
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;

	bool argsOK = false;

	if (argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiii", &centerX, &centerY, &radius, &startAngle, &spanAngle, &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &centerX, &centerY, &radius, &startAngle, &spanAngle, &r, &g, &b))
	{
		argsOK = true;
	}
	if (argCount == 7 && PyArg_ParseTuple(args, "iiiiisO", &centerX, &centerY, &radius, &startAngle, &spanAngle, &brush, &bytearray))
	{
		argsOK = true;
	}
	if (argCount == 5 && PyArg_ParseTuple(args, "iiisO", &centerX, &centerY, &radius, &brush, &bytearray))
	{
		argsOK = true;
	}

	if (!argsOK)
	{
		if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
		return nullptr;
	}

	QPainter* painter = getEffect()->_painter.get();
	startAngle = qMax(qMin(startAngle, 360), 0);
	spanAngle = qMax(qMin(spanAngle, 360), -360);

	if (argCount == 7 || argCount == 5)
	{
		if (!PyByteArray_Check(bytearray))
		{
			PyErr_SetString(PyExc_RuntimeError, "Last argument is not a bytearray");
			return nullptr;
		}
		Py_ssize_t length = PyByteArray_Size(bytearray);
		if (length % 5 != 0)
		{
			PyErr_SetString(PyExc_RuntimeError, "Length of bytearray argument should multiple of 5");
			return nullptr;
		}

			QConicalGradient gradient(QPoint(centerX, centerY), startAngle);
		const char* data = PyByteArray_AS_STRING(bytearray);

		for (int idx = 0; idx < length; idx += 5)
		{
			gradient.setColorAt(
				((uint8_t)data[idx]) / 255.0,
				QColor(
					(uint8_t)(data[idx + 1]),
					(uint8_t)(data[idx + 2]),
					(uint8_t)(data[idx + 3]),
					(uint8_t)(data[idx + 4])
				));
		}
		painter->setBrush(gradient);
		Py_RETURN_NONE;
	}

	painter->setBrush(QBrush(QColor(r, g, b, a), Qt::SolidPattern));
	QPen oldPen = painter->pen();
	QPen newPen(QColor(r, g, b, a));
	painter->setPen(newPen);
	painter->drawPie(centerX - radius, centerY - radius, centerX + radius, centerY + radius, startAngle * 16, spanAngle * 16);
	painter->setPen(oldPen);
	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapImageSolidFill(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int width = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

	bool argsOK = false;

	if (argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &startX, &startY, &width, &height, &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 7 && PyArg_ParseTuple(args, "iiiiiii", &startX, &startY, &width, &height, &r, &g, &b))
	{
		argsOK = true;
	}
	if (argCount == 4 && PyArg_ParseTuple(args, "iiii", &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 3 && PyArg_ParseTuple(args, "iii", &r, &g, &b))
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QRect myQRect(startX, startY, width, height);
		getEffect()->_painter->fillRect(myQRect, QColor(r, g, b, a));
		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}


PyObject* EffectModule::wrapImageDrawLine(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int thick = 1;
	int endX = getEffect()->_imageSize.width();
	int endY = getEffect()->_imageSize.height();

	bool argsOK = false;

	if (argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiii", &startX, &startY, &endX, &endY, &thick, &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &startX, &startY, &endX, &endY, &thick, &r, &g, &b))
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter* painter = getEffect()->_painter.get();
		QRect myQRect(startX, startY, endX, endY);
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r, g, b, a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawLine(startX, startY, endX, endY);
		painter->setPen(oldPen);

		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}

PyObject* EffectModule::wrapImageDrawPoint(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int x = 0;
	int y = 0;
	int a = 255;
	int thick = 1;

	bool argsOK = false;

	if (argCount == 7 && PyArg_ParseTuple(args, "iiiiiii", &x, &y, &thick, &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 6 && PyArg_ParseTuple(args, "iiiiii", &x, &y, &thick, &r, &g, &b))
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter* painter = getEffect()->_painter.get();
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r, g, b, a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawPoint(x, y);
		painter->setPen(oldPen);

		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}

PyObject* EffectModule::wrapImageDrawRect(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int thick = 1;
	int width = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

	bool argsOK = false;

	if (argCount == 9 && PyArg_ParseTuple(args, "iiiiiiiii", &startX, &startY, &width, &height, &thick, &r, &g, &b, &a))
	{
		argsOK = true;
	}
	if (argCount == 8 && PyArg_ParseTuple(args, "iiiiiiii", &startX, &startY, &width, &height, &thick, &r, &g, &b))
	{
		argsOK = true;
	}

	if (argsOK)
	{
		QPainter* painter = getEffect()->_painter.get();
		QRect myQRect(startX, startY, width, height);
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r, g, b, a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawRect(startX, startY, width, height);
		painter->setPen(oldPen);

		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}


PyObject* EffectModule::wrapImageSetPixel(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int x = 0;
	int y = 0;

	if (argCount == 5 && PyArg_ParseTuple(args, "iiiii", &x, &y, &r, &g, &b))
	{
		getEffect()->_image.setPixel(x, y, qRgb(r, g, b));
		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}


PyObject* EffectModule::wrapImageGetPixel(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int x = 0;
	int y = 0;

	if (argCount == 2 && PyArg_ParseTuple(args, "ii", &x, &y))
	{
		QRgb rgb = getEffect()->_image.pixel(x, y);
		return Py_BuildValue("iii", qRed(rgb), qGreen(rgb), qBlue(rgb));
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}

PyObject* EffectModule::wrapImageSave(PyObject* /*self*/, PyObject* /*args*/)
{
	QImage img(getEffect()->_image.copy());
	getEffect()->_imageStack.append(img);

	return Py_BuildValue("i", getEffect()->_imageStack.size() - 1);
}

PyObject* EffectModule::wrapImageMinSize(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int w = 0;
	int h = 0;
	int width = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

	if (argCount == 2 && PyArg_ParseTuple(args, "ii", &w, &h))
	{
		if (width < w || height < h)
		{
			// End the active paint session before scaling (new QPainter must not start while old one is active)
			getEffect()->_painter.reset();
			getEffect()->_image = getEffect()->_image.scaled(qMax(width, w), qMax(height, h), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
			getEffect()->_imageSize = getEffect()->_image.size();
			getEffect()->_painter.reset(new QPainter(&(getEffect()->_image)));
		}
		return Py_BuildValue("ii", getEffect()->_image.width(), getEffect()->_image.height());
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}

PyObject* EffectModule::wrapImageWidth(PyObject* /*self*/, PyObject* /*args*/)
{
	return Py_BuildValue("i", getEffect()->_imageSize.width());
}

PyObject* EffectModule::wrapImageHeight(PyObject* /*self*/, PyObject* /*args*/)
{
	return Py_BuildValue("i", getEffect()->_imageSize.height());
}

PyObject* EffectModule::wrapImageCRotate(PyObject* /*self*/, PyObject* args)
{
	Py_ssize_t argCount = PyTuple_Size(args);
	int angle;

	if (argCount == 1 && PyArg_ParseTuple(args, "i", &angle))
	{
		angle = qMax(qMin(angle, 360), 0);
		getEffect()->_painter->rotate(angle);
		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}

PyObject* EffectModule::wrapImageCOffset(PyObject* /*self*/, PyObject* args)
{
	int offsetX = 0;
	int offsetY = 0;
	Py_ssize_t argCount = PyTuple_Size(args);

	if (argCount == 2)
	{
		PyArg_ParseTuple(args, "ii", &offsetX, &offsetY);
	}

	getEffect()->_painter->translate(QPoint(offsetX, offsetY));
	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapImageCShear(PyObject* /*self*/, PyObject* args)
{
	int sh = 0;
	int sv = 0;
	Py_ssize_t argCount = PyTuple_Size(args);

	if (argCount == 2 && PyArg_ParseTuple(args, "ii", &sh, &sv))
	{
		getEffect()->_painter->shear(sh, sv);
		Py_RETURN_NONE;
	}

	if (!PyErr_Occurred()) { PyErr_SetString(PyExc_TypeError, "Invalid argument count or type."); }
	return nullptr;
}

PyObject* EffectModule::wrapImageResetT(PyObject* /*self*/, PyObject* /*args*/)
{
	getEffect()->_painter->resetTransform();
	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapLowestUpdateInterval(PyObject* /*self*/, PyObject* /*args*/)
{
	return Py_BuildValue("d", getEffect()->_lowestUpdateIntervalInSeconds);
}

PyObject* EffectModule::wrapImageStackClear(PyObject* /*self*/, PyObject* /*args*/)
{
	getEffect()->_imageStack.clear();
	Py_RETURN_NONE;
}
