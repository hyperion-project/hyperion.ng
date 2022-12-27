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

// Get the effect from the capsule
#define getEffect() static_cast<Effect*>((Effect*)PyCapsule_Import("hyperion.__effectObj", 0))

// create the hyperion module
struct PyModuleDef EffectModule::moduleDef = {
	PyModuleDef_HEAD_INIT,
	"hyperion",            /* m_name */
	"Hyperion module",     /* m_doc */
	-1,                    /* m_size */
	EffectModule::effectMethods, /* m_methods */
	NULL,                  /* m_reload */
	NULL,                  /* m_traverse */
	NULL,                  /* m_clear */
	NULL,                  /* m_free */
};

PyObject* EffectModule::PyInit_hyperion()
{
	return PyModule_Create(&moduleDef);
}

void EffectModule::registerHyperionExtensionModule()
{
	PyImport_AppendInittab("hyperion", &PyInit_hyperion);
}

PyObject *EffectModule::json2python(const QJsonValue &jsonData)
{
	switch (jsonData.type())
	{
		case QJsonValue::Null:
			Py_RETURN_NONE;
		case QJsonValue::Undefined:
			Py_RETURN_NOTIMPLEMENTED;
		case QJsonValue::Double:
		{
			double doubleIntegratlPart;
			double doubleFractionalPart = std::modf(jsonData.toDouble(), &doubleIntegratlPart);
			if (doubleFractionalPart > std::numeric_limits<double>::epsilon())
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
	Py_RETURN_NONE;
}

// Python method table
PyMethodDef EffectModule::effectMethods[] = {
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
	{NULL, NULL, 0, NULL}
};

PyObject* EffectModule::wrapSetColor(PyObject *self, PyObject *args)
{
	// check the number of arguments
	int argCount = PyTuple_Size(args);
	if (argCount == 3)
	{
		// three separate arguments for red, green, and blue
		ColorRgb color;
		if (PyArg_ParseTuple(args, "bbb", &color.red, &color.green, &color.blue))
		{
			getEffect()->_colors.fill(color);
			QVector<ColorRgb> _cQV = getEffect()->_colors;
			emit getEffect()->setInput(getEffect()->_priority, std::vector<ColorRgb>( _cQV.begin(), _cQV.end() ), getEffect()->getRemaining(), false);
			Py_RETURN_NONE;
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
				if (length == 3 * static_cast<size_t>(getEffect()->_hyperion->getLedCount()))
				{
					char * data = PyByteArray_AS_STRING(bytearray);
					memcpy(getEffect()->_colors.data(), data, length);
					QVector<ColorRgb> _cQV = getEffect()->_colors;
					emit getEffect()->setInput(getEffect()->_priority, std::vector<ColorRgb>( _cQV.begin(), _cQV.end() ), getEffect()->getRemaining(), false);
					Py_RETURN_NONE;
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
}

PyObject* EffectModule::wrapSetImage(PyObject *self, PyObject *args)
{
	// bytearray of values
	int width = 0;
	int height = 0;
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

PyObject* EffectModule::wrapGetImage(PyObject *self, PyObject *args)
{
	QBuffer buffer;
	QImageReader reader;
	char *source;
	int cropLeft = 0, cropTop = 0, cropRight = 0, cropBottom = 0;
	int grayscale = false;

	if (getEffect()->_imageData.isEmpty())
	{
		Q_INIT_RESOURCE(EffectEngine);

		if(!PyArg_ParseTuple(args, "s|iiiip", &source, &cropLeft, &cropTop, &cropRight, &cropBottom, &grayscale))
		{
			PyErr_SetString(PyExc_TypeError, "String required");
			return nullptr;
		}

		const QUrl url = QUrl(source);
		if (url.isValid())
		{
			QNetworkAccessManager *networkManager = new QNetworkAccessManager();
			QNetworkReply * networkReply = networkManager->get(QNetworkRequest(url));

			QEventLoop eventLoop;
			connect(networkReply, &QNetworkReply::finished, &eventLoop, &QEventLoop::quit);
			eventLoop.exec();

			if (networkReply->error() == QNetworkReply::NoError)
			{
				buffer.setData(networkReply->readAll());
				buffer.open(QBuffer::ReadOnly);
				reader.setDecideFormatFromContent(true);
				reader.setDevice(&buffer);
			}

			delete networkReply;
    		delete networkManager;
		}
		else
		{
			QString file = QString::fromUtf8(source);

			if (file.mid(0, 1)  == ":")
				file = ":/effects/"+file.mid(1);

			reader.setDecideFormatFromContent(true);
			reader.setFileName(file);
		}
	}
	else
	{
		PyArg_ParseTuple(args, "|siiiip", &source, &cropLeft, &cropTop, &cropRight, &cropBottom, &grayscale);
		buffer.setData(QByteArray::fromBase64(getEffect()->_imageData.toUtf8()));
		buffer.open(QBuffer::ReadOnly);
		reader.setDecideFormatFromContent(true);
		reader.setDevice(&buffer);
	}

	if (reader.canRead())
	{
		PyObject *result = PyList_New(reader.imageCount());

		for (int i = 0; i < reader.imageCount(); ++i)
		{
			reader.jumpToImage(i);
			if (reader.canRead())
			{
				QImage qimage = reader.read();
				int width = qimage.width();
				int height = qimage.height();

				if (cropLeft > 0 || cropTop > 0 || cropRight > 0 || cropBottom > 0)
				{
					if (cropLeft + cropRight >= width || cropTop + cropBottom >= height)
					{
						QString errorStr = QString("Rejecting invalid crop values: left: %1, right: %2, top: %3, bottom: %4, higher than height/width %5/%6").arg(cropLeft).arg(cropRight).arg(cropTop).arg(cropBottom).arg(height).arg(width);
						PyErr_SetString(PyExc_RuntimeError, qPrintable(errorStr));
						return nullptr;
					}

					qimage = qimage.copy(cropLeft, cropTop, width - cropLeft - cropRight, height - cropTop - cropBottom);
					width = qimage.width();
					height = qimage.height();
				}

				QByteArray binaryImage;
				for (int i = 0; i<height; i++)
				{
					const QRgb *scanline = reinterpret_cast<const QRgb *>(qimage.scanLine(i));
					const QRgb *end = scanline + qimage.width();
					for (; scanline != end; scanline++)
					{
						binaryImage.append(!grayscale ? (char) qRed(scanline[0]) : (char) qGray(scanline[0]));
						binaryImage.append(!grayscale ? (char) qGreen(scanline[1]) : (char) qGray(scanline[1]));
						binaryImage.append(!grayscale ? (char) qBlue(scanline[2]) : (char) qGray(scanline[2]));
					}
				}
				PyList_SET_ITEM(result, i, Py_BuildValue("{s:i,s:i,s:O}", "imageWidth", width, "imageHeight", height, "imageData", PyByteArray_FromStringAndSize(binaryImage.constData(),binaryImage.size())));
			}
			else
			{
				PyErr_SetString(PyExc_TypeError, reader.errorString().toUtf8().constData());
				return nullptr;
			}
		}

		return result;
	}
	else
	{
		PyErr_SetString(PyExc_TypeError, reader.errorString().toUtf8().constData());
		return nullptr;
	}
}

PyObject* EffectModule::wrapAbort(PyObject *self, PyObject *)
{
	return Py_BuildValue("i", getEffect()->isInterruptionRequested() ? 1 : 0);
}


PyObject* EffectModule::wrapImageShow(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int imgId = -1;
	bool argsOk = (argCount == 0);
	if (argCount == 1 && PyArg_ParseTuple(args, "i", &imgId))
	{
		argsOk = true;
	}

	if ( ! argsOk || (imgId>-1 && imgId >= getEffect()->_imageStack.size()))
	{
		return nullptr;
	}


	QImage * qimage = (imgId<0) ? &(getEffect()->_image) : &(getEffect()->_imageStack[imgId]);
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
	emit getEffect()->setInputImage(getEffect()->_priority, image, getEffect()->getRemaining(), false);

	return Py_BuildValue("");
}

PyObject* EffectModule::wrapImageLinearGradient(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	PyObject * bytearray = nullptr;
	int startRX = 0;
	int startRY = 0;
	int startX = 0;
	int startY = 0;
	int width = getEffect()->_imageSize.width();
	int endX {width};
	int height = getEffect()->_imageSize.height();
	int endY {height};
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
				getEffect()->_painter->fillRect(myQRect, gradient);

				Py_RETURN_NONE;
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

PyObject* EffectModule::wrapImageConicalGradient(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	PyObject * bytearray = nullptr;
	int centerX = 0;
	int centerY = 0;
	int angle = 0;
	int startX = 0;
	int startY = 0;
	int width  = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

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

				getEffect()->_painter->fillRect(myQRect, gradient);

				Py_RETURN_NONE;
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


PyObject* EffectModule::wrapImageRadialGradient(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	PyObject * bytearray = nullptr;
	int centerX = 0;
	int centerY = 0;
	int radius = 0;
	int focalX = 0;
	int focalY = 0;
	int focalRadius =0;
	int spread = 0;
	int startX = 0;
	int startY = 0;
	int width  = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

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
				getEffect()->_painter->fillRect(myQRect, gradient);

				Py_RETURN_NONE;
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

PyObject* EffectModule::wrapImageDrawPolygon(PyObject *self, PyObject *args)
{
	PyObject * bytearray = nullptr;

	int argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
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

				QPainter * painter = getEffect()->_painter;
				QPen oldPen = painter->pen();
				QPen newPen(QColor(r,g,b,a));
				painter->setPen(newPen);
				painter->setBrush(QBrush(QColor(r,g,b,a), Qt::SolidPattern));
				painter->drawPolygon(points);
				painter->setPen(oldPen);
				Py_RETURN_NONE;
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

PyObject* EffectModule::wrapImageDrawPie(PyObject *self, PyObject *args)
{
	PyObject * bytearray = nullptr;

	QString brush;
	int argCount = PyTuple_Size(args);
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
		QPainter * painter = getEffect()->_painter;
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

					Py_RETURN_NONE;
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
		Py_RETURN_NONE;
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageSolidFill(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int width  = getEffect()->_imageSize.width();
	int height = getEffect()->_imageSize.height();

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
		getEffect()->_painter->fillRect(myQRect, QColor(r,g,b,a));
		Py_RETURN_NONE;
	}
	return nullptr;
}


PyObject* EffectModule::wrapImageDrawLine(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int thick  = 1;
	int endX   = getEffect()->_imageSize.width();
	int endY   = getEffect()->_imageSize.height();

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
		QPainter * painter = getEffect()->_painter;
		QRect myQRect(startX, startY, endX, endY);
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawLine(startX, startY, endX, endY);
		painter->setPen(oldPen);

		Py_RETURN_NONE;
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageDrawPoint(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int x = 0;
	int y = 0;
	int a = 255;
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
		QPainter * painter = getEffect()->_painter;
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawPoint(x, y);
		painter->setPen(oldPen);

		Py_RETURN_NONE;
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageDrawRect(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int a = 255;
	int startX = 0;
	int startY = 0;
	int thick  = 1;
	int width   = getEffect()->_imageSize.width();
	int height  = getEffect()->_imageSize.height();

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
		QPainter * painter = getEffect()->_painter;
		QRect myQRect(startX,startY,width,height);
		QPen oldPen = painter->pen();
		QPen newPen(QColor(r,g,b,a));
		newPen.setWidth(thick);
		painter->setPen(newPen);
		painter->drawRect(startX, startY, width, height);
		painter->setPen(oldPen);

		Py_RETURN_NONE;
	}
	return nullptr;
}


PyObject* EffectModule::wrapImageSetPixel(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int r = 0;
	int g = 0;
	int b = 0;
	int x = 0;
	int y = 0;

	if ( argCount == 5 && PyArg_ParseTuple(args, "iiiii", &x, &y, &r, &g, &b ) )
	{
		getEffect()->_image.setPixel(x,y,qRgb(r,g,b));
		Py_RETURN_NONE;
	}

	return nullptr;
}


PyObject* EffectModule::wrapImageGetPixel(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int x = 0;
	int y = 0;

	if ( argCount == 2 && PyArg_ParseTuple(args, "ii", &x, &y) )
	{
		QRgb rgb = getEffect()->_image.pixel(x,y);
		return Py_BuildValue("iii",qRed(rgb),qGreen(rgb),qBlue(rgb));
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageSave(PyObject *self, PyObject *args)
{
	QImage img(getEffect()->_image.copy());
	getEffect()->_imageStack.append(img);

	return Py_BuildValue("i", getEffect()->_imageStack.size()-1);
}

PyObject* EffectModule::wrapImageMinSize(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int w = 0;
	int h = 0;
	int width   = getEffect()->_imageSize.width();
	int height  = getEffect()->_imageSize.height();

	if ( argCount == 2 && PyArg_ParseTuple(args, "ii", &w, &h) )
	{
		if (width<w || height<h)
		{
			delete getEffect()->_painter;

			getEffect()->_image = getEffect()->_image.scaled(qMax(width,w),qMax(height,h), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
			getEffect()->_imageSize = getEffect()->_image.size();
			getEffect()->_painter = new QPainter(&(getEffect()->_image));
		}
		return Py_BuildValue("ii", getEffect()->_image.width(), getEffect()->_image.height());
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageWidth(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", getEffect()->_imageSize.width());
}

PyObject* EffectModule::wrapImageHeight(PyObject *self, PyObject *args)
{
	return Py_BuildValue("i", getEffect()->_imageSize.height());
}

PyObject* EffectModule::wrapImageCRotate(PyObject *self, PyObject *args)
{
	int argCount = PyTuple_Size(args);
	int angle;

	if ( argCount == 1 && PyArg_ParseTuple(args, "i", &angle ) )
	{
		angle = qMax(qMin(angle,360),0);
		getEffect()->_painter->rotate(angle);
		Py_RETURN_NONE;
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageCOffset(PyObject *self, PyObject *args)
{
	int offsetX = 0;
	int offsetY = 0;
	int argCount = PyTuple_Size(args);

	if ( argCount == 2 )
	{
		PyArg_ParseTuple(args, "ii", &offsetX, &offsetY );
	}

	getEffect()->_painter->translate(QPoint(offsetX,offsetY));
	Py_RETURN_NONE;
}

PyObject* EffectModule::wrapImageCShear(PyObject *self, PyObject *args)
{
	int sh = 0;
	int sv = 0;
	int argCount = PyTuple_Size(args);

	if ( argCount == 2 && PyArg_ParseTuple(args, "ii", &sh, &sv ))
	{
		getEffect()->_painter->shear(sh,sv);
		Py_RETURN_NONE;
	}
	return nullptr;
}

PyObject* EffectModule::wrapImageResetT(PyObject *self, PyObject *args)
{
	getEffect()->_painter->resetTransform();
	Py_RETURN_NONE;
}
