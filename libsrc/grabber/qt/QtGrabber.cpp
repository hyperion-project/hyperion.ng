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

QtGrabber::QtGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display)
	: Grabber("QTGRABBER", 0, 0, cropLeft, cropRight, cropTop, cropBottom)
	, _display(display)
	, _pixelDecimation(pixelDecimation)
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

	if (getenv("WAYLAND_DISPLAY") != nullptr)
	{
		_isWayland = true;
	}
	else
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
				Info(_log,"Display %d: Name:%s Geometry: (L,T,R,B) %d,%d,%d,%d Depth:%dbit", index, QSTRING_CSTR(screen->name()), geo.left(), geo.top() ,geo.right(), geo.bottom(), screen->depth());
				++index;
			}

			_isVirtual = false;
			// be sure the index is available
			if (_display > _numberOfSDisplays - 1 )
			{

				if (screens.at(0)->size() != screens.at(0)->virtualSize())
				{
					Info(_log, "Using virtual display across all screens");
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

			Info(_log,"Initialized display %d", _display);
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
	if (!_enabled)
	{
		return 0;
	}

	if(_screen == nullptr)
	{
		// reinit, this will disable capture on failure
		bool result = setupDisplay();
		setEnabled(result);
		return -1;
	}

	QPixmap originalPixmap = _screen->grabWindow(0, _src_x, _src_y, _src_x_max, _src_y_max);
	QImage imageFrame = originalPixmap.toImage().scaled(_calculatedWidth, _calculatedHeight).convertToFormat( QImage::Format_RGB888);
	image.resize(static_cast<uint>(_calculatedWidth), static_cast<uint>(_calculatedHeight));

	for (int y = 0; y < imageFrame.height(); y++)
	{
		memcpy((unsigned char*)image.memptr() + y * image.width() * 3, static_cast<unsigned char*>(imageFrame.scanLine(y)), imageFrame.width() * 3);
	}

	return 0;
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

void QtGrabber::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	updateScreenDimensions(true);
}

void QtGrabber::setDisplayIndex(int index)
{
	if (_display != index)
	{
		if (index <= _numberOfSDisplays)
		{
			_display = index;
		}
		else {
			_display = 0;
		}
		setupDisplay();
	}
}

QJsonObject QtGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;
	if ( open() )
	{
		QList<QScreen*> screens = QGuiApplication::screens();

		inputsDiscovered["device"] = "qt";
		inputsDiscovered["device_name"] = "QT";
		inputsDiscovered["type"] = "screen";

		QJsonArray video_inputs;

		if (!screens.isEmpty())
		{
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
		}
		else
		{
			DebugIf(verbose, _log, "No displays found to capture from!");
		}
	}
	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;

}
