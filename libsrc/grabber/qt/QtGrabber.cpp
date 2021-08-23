// proj
#include <grabber/QtGrabber.h>

// qt
#include <QPixmap>
#include <QWindow>
#include <QGuiApplication>
#include <QWidget>
#include <QScreen>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// Constants
namespace {
const bool verbose = false;
} //End of constants

QtGrabber::QtGrabber(int display, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("QTGRABBER", cropLeft, cropRight, cropTop, cropBottom)
	  , _display(display)
	  , _calculatedWidth(0)
	  , _calculatedHeight(0)
	  , _src_x(0)
	  , _src_y(0)
	  , _src_x_max(0)
	  , _src_y_max(0)
	  , _isWayland(false)
	  , _screen(nullptr)
	  , _isVirtual(false)
{
	_logger = Logger::getInstance("Qt");
	_useImageResampler = false;
}

QtGrabber::~QtGrabber()
{
	freeResources();
}

void QtGrabber::freeResources()
{
	// Qt seems to hold the ownership of the QScreen pointers
}

bool QtGrabber::open()
{
	bool rc = false;

#ifndef _WIN32
	if (getenv("WAYLAND_DISPLAY") != nullptr)
	{
		_isWayland = true;
	}
	else
#endif
	{
		rc = true;
	}
	return rc;
}

bool QtGrabber::setupDisplay()
{
	bool result = false;
	if ( ! open() )
	{
		if ( _isWayland  )
		{
			Error(_log, "Grabber does not work under Wayland!");
		}
	}
	else
	{
		// cleanup last screen
		freeResources();
		_numberOfSDisplays = 0;

		QScreen* primary = QGuiApplication::primaryScreen();
		QList<QScreen *> screens = QGuiApplication::screens();
		// inject main screen at 0, if not nullptr
		if(primary != nullptr)
		{
			screens.prepend(primary);
			// remove last main screen if twice in list
			if(screens.lastIndexOf(primary) > 0)
			{
				screens.removeAt(screens.lastIndexOf(primary));
			}
		}

		if(screens.isEmpty())
		{
			Error(_log, "No displays found to capture from!");
			result =  false;
		}
		else
		{
			_numberOfSDisplays = screens.size();

			Info(_log,"Available Displays:");
			int index = 0;
			for(auto * screen : qAsConst(screens))
			{
				const QRect geo = screen->geometry();
				Info(_log,"Display %d: Name: %s Geometry: (L,T,R,B) %d,%d,%d,%d Depth:%dbit", index, QSTRING_CSTR(screen->name()), geo.left(), geo.top() ,geo.right(), geo.bottom(), screen->depth());
				++index;
			}

			if (screens.at(0)->size() != screens.at(0)->virtualSize())
			{
				const QRect vgeo = screens.at(0)->virtualGeometry();
				Info(_log,"Display %d: Name: %s Geometry: (L,T,R,B) %d,%d,%d,%d Depth:%dbit", _numberOfSDisplays, "All Displays", vgeo.left(), vgeo.top() ,vgeo.right(), vgeo.bottom(), screens.at(0)->depth());
			}

			_isVirtual = false;
			// be sure the index is available
			if (_display > _numberOfSDisplays - 1 )
			{

				if ((screens.at(0)->size() != screens.at(0)->virtualSize()) && (_display == _numberOfSDisplays))
				{
					_isVirtual = true;
					_display = 0;

				}
				else
				{
					Info(_log, "The requested display index '%d' is not available, falling back to display 0", _display);
					_display = 0;
				}
			}

			// init the requested display
			_screen = screens.at(_display);
			connect(_screen, &QScreen::geometryChanged, this, &QtGrabber::geometryChanged);
			updateScreenDimensions(true);

			if (_isVirtual)
			{
				Info(_log, "Using virtual display across all screens");
			}
			else
			{
				Info(_log,"Initialized display %d", _display);
			}
			result =  true;
		}
	}
	return result;
}

void QtGrabber::geometryChanged(const QRect &geo)
{
	Info(_log, "The current display changed geometry to (L,T,R,B) %d,%d,%d,%d", geo.left(), geo.top() ,geo.right(), geo.bottom());
	updateScreenDimensions(true);
}

int QtGrabber::grabFrame(Image<ColorRgb> & image)
{
	int rc = 0;
	if (_isEnabled && !_isDeviceInError)
	{
		if(_screen == nullptr)
		{
			// reinit, this will disable capture on failure
			bool result = setupDisplay();
			setEnabled(result);
		}

		if (_isEnabled)
		{
			QPixmap originalPixmap = _screen->grabWindow(0, _src_x, _src_y, _src_x_max, _src_y_max);
			if (originalPixmap.isNull())
			{
				rc = -1;
			}
			else
			{
				QImage imageFrame = originalPixmap.toImage().scaled(_calculatedWidth, _calculatedHeight).convertToFormat( QImage::Format_RGB888);
				image.resize(static_cast<uint>(_calculatedWidth), static_cast<uint>(_calculatedHeight));

				for (int y = 0; y < imageFrame.height(); y++)
				{
					memcpy((unsigned char*)image.memptr() + y * image.width() * 3, static_cast<unsigned char*>(imageFrame.scanLine(y)), imageFrame.width() * 3);
				}
			}
		}
	}
	return rc;
}

int QtGrabber::updateScreenDimensions(bool force)
{
	if(_screen == nullptr)
	{
		return -1;
	}

	QRect geo;

	if (_isVirtual)
	{
		geo = _screen->virtualGeometry();
	}
	else
	{
		geo = _screen->geometry();
	}
	if (!force && _width == geo.width() && _height == geo.height())
	{
		// No update required
		return 0;
	}

	Info(_log, "Update of screen resolution: [%dx%d] to [%dx%d]", _width, _height, geo.width(), geo.height());
	_width  = geo.width();
	_height = geo.height();

	int width=0;
	int height=0;

	// Image scaling is performed by Qt
	width  =  (_width > (_cropLeft + _cropRight))
				? ((_width - _cropLeft - _cropRight) / _pixelDecimation)
				: (_width / _pixelDecimation);

	height =  (_height > (_cropTop + _cropBottom))
				 ? ((_height - _cropTop - _cropBottom) / _pixelDecimation)
				 : (_height / _pixelDecimation);


	// calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		_calculatedWidth  = width /2;
		_calculatedHeight = height;
		_src_x  = _cropLeft / 2;
		_src_y  = _cropTop;
		_src_x_max = (_width / 2) - _cropRight - _cropLeft;
		_src_y_max = _height - _cropBottom - _cropTop;
		break;
	case VideoMode::VIDEO_3DTAB:
		_calculatedWidth  = width;
		_calculatedHeight = height / 2;
		_src_x  = _cropLeft;
		_src_y  = _cropTop / 2;
		_src_x_max = _width - _cropRight - _cropLeft;
		_src_y_max = (_height / 2) - _cropBottom - _cropTop;
		break;
	case VideoMode::VIDEO_2D:
	default:
		_calculatedWidth  = width;
		_calculatedHeight = height;
		_src_x  = _cropLeft;
		_src_y  = _cropTop;
		_src_x_max = _width - _cropRight - _cropLeft;
		_src_y_max = _height - _cropBottom - _cropTop;
		break;
	}

	Info(_log, "Update output image resolution to [%dx%d]", _calculatedWidth, _calculatedHeight);
	return 1;
}

void QtGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	updateScreenDimensions(true);
}

bool QtGrabber::setPixelDecimation(int pixelDecimation)
{
	bool rc (true);
	if(Grabber::setPixelDecimation(pixelDecimation))
	{
		if ( updateScreenDimensions(true) < 0)
		{
			rc = false;
		}
	}
	return rc;
}

void QtGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	updateScreenDimensions(true);
}

bool QtGrabber::setDisplayIndex(int index)
{
	bool rc (true);
	if (_display != index)
	{
		if (index <= _numberOfSDisplays)
		{
			_display = index;
		}
		else {
			_display = 0;
		}
		rc = setupDisplay();
	}
	return rc;
}

QJsonObject QtGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;
	if ( open() )
	{
		QList<QScreen*> screens = QGuiApplication::screens();
		if (!screens.isEmpty())
		{
			inputsDiscovered["device"] = "qt";
			inputsDiscovered["device_name"] = "QT";
			inputsDiscovered["type"] = "screen";

			QJsonArray video_inputs;
			QJsonArray fps = { 1, 5, 10, 15, 20, 25, 30, 40, 50, 60 };

			for (int i = 0; i < screens.size(); ++i)
			{
				QJsonObject in;

				QString name = screens.at(i)->name();
				int pos = name.lastIndexOf('\\');
				if (pos != -1)
				{
					name = name.right(name.length()-pos-1);
				}

				in["name"] = name;
				in["inputIdx"] = i;

				QJsonArray formats;
				QJsonObject format;

				QJsonArray resolutionArray;

				QJsonObject resolution;

				resolution["width"] = screens.at(i)->size().width();
				resolution["height"] = screens.at(i)->size().height();
				resolution["fps"] = fps;

				resolutionArray.append(resolution);

				format["resolutions"] = resolutionArray;
				formats.append(format);

				in["formats"] = formats;
				video_inputs.append(in);
			}

			if (screens.at(0)->size() != screens.at(0)->virtualSize())
			{
				QJsonObject in;
				in["name"] = "All Displays";
				in["inputIdx"] = screens.size();
				in["virtual"] = true;

				QJsonArray formats;
				QJsonObject format;

				QJsonArray resolutionArray;

				QJsonObject resolution;

				resolution["width"] = screens.at(0)->virtualSize().width();
				resolution["height"] = screens.at(0)->virtualSize().height();
				resolution["fps"] = fps;

				resolutionArray.append(resolution);

				format["resolutions"] = resolutionArray;
				formats.append(format);

				in["formats"] = formats;
				video_inputs.append(in);
			}
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = _fps;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}

		if (inputsDiscovered.isEmpty())
		{
			DebugIf(verbose, _log, "No displays found to capture from!");
		}
	}
	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;

}
