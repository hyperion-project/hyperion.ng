// proj
#include <grabber/QtGrabber.h>

// qt
#include <QPixmap>
#include <QWindow>
#include <QGuiApplication>
#include <QWidget>
#include <QScreen>

QtGrabber::QtGrabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display)
	: Grabber("QTGRABBER", 0, 0, cropLeft, cropRight, cropTop, cropBottom)
	, _display(unsigned(display))
	, _pixelDecimation(pixelDecimation)
	, _screenWidth(0)
	, _screenHeight(0)
	, _src_x(0)
	, _src_y(0)
	, _src_x_max(0)
	, _src_y_max(0)
	, _screen(nullptr)
{
	_useImageResampler = false;

	// init
	setupDisplay();
}

QtGrabber::~QtGrabber()
{
	freeResources();
}

void QtGrabber::freeResources()
{
	// Qt seems to hold the ownership of the QScreen pointers
}

bool QtGrabber::setupDisplay()
{
	// cleanup last screen
	freeResources();

	QScreen* primary = QGuiApplication::primaryScreen();
	QList<QScreen *> screens = QGuiApplication::screens();
	// inject main screen at 0, if not nullptr
	if(primary != nullptr)
	{
		screens.prepend(primary);
		// remove last main screen if twice in list
		if(screens.lastIndexOf(primary) > 0)
			screens.removeAt(screens.lastIndexOf(primary));
	}

	if(screens.isEmpty())
	{
		Error(_log, "No displays found to capture from!");
		return false;
	}

	Info(_log,"Available Displays:");
	int index = 0;
	for(auto screen : screens)
	{
		const QRect geo = screen->geometry();
		Info(_log,"Display %d: Name:%s Geometry: (L,T,R,B) %d,%d,%d,%d Depth:%dbit", index, QSTRING_CSTR(screen->name()), geo.left(), geo.top() ,geo.right(), geo.bottom(), screen->depth());
		index++;
	}

	// be sure the index is available
	if(_display > unsigned(screens.size()-1))
	{
		Info(_log, "The requested display index '%d' is not available, falling back to display 0", _display);
		_display = 0;
	}

	// init the requested display
	_screen = screens.at(_display);
	connect(_screen, &QScreen::geometryChanged, this, &QtGrabber::geometryChanged);
	updateScreenDimensions(true);

	Info(_log,"Initialized display %d", _display);
	return true;
}

void QtGrabber::geometryChanged(const QRect &geo)
{
	Info(_log, "The current display changed geometry to (L,T,R,B) %d,%d,%d,%d", geo.left(), geo.top() ,geo.right(), geo.bottom());
	updateScreenDimensions(true);
}

int QtGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;
	if(_screen == nullptr)
	{
		// reinit, this will disable capture on failure
		setEnabled(setupDisplay());
		return -1;
	}
	QPixmap originalPixmap = _screen->grabWindow(0, _src_x, _src_y, _src_x_max, _src_y_max);
	QPixmap resizedPixmap = originalPixmap.scaled(_width,_height);
	QImage imageFrame = resizedPixmap.toImage().convertToFormat( QImage::Format_RGB888);
	image.resize(imageFrame.width(), imageFrame.height());

	for (int y=0; y<imageFrame.height(); ++y)
		for (int x=0; x<imageFrame.width(); ++x)
		{
			QColor inPixel(imageFrame.pixel(x,y));
			ColorRgb & outPixel = image(x,y);
			outPixel.red   = inPixel.red();
			outPixel.green = inPixel.green();
			outPixel.blue  = inPixel.blue();
		}

	return 0;
}

int QtGrabber::updateScreenDimensions(bool force)
{
	if(!_screen)
		return -1;

	const QRect& geo = _screen->geometry();
	if (!force && _screenWidth == unsigned(geo.right()) && _screenHeight == unsigned(geo.bottom()))
	{
		// No update required
		return 0;
	}

	Info(_log, "Update of screen resolution: [%dx%d] to [%dx%d]", _screenWidth, _screenHeight, geo.right(), geo.bottom());
	_screenWidth  = geo.right() - geo.left();
	_screenHeight = geo.bottom() - geo.top();

	int width=0, height=0;

	// Image scaling is performed by Qt
	width  =  (_screenWidth > unsigned(_cropLeft + _cropRight))
		? ((_screenWidth - _cropLeft - _cropRight) / _pixelDecimation)
		: (_screenWidth / _pixelDecimation);

	height =  (_screenHeight > unsigned(_cropTop + _cropBottom))
		? ((_screenHeight - _cropTop - _cropBottom) / _pixelDecimation)
		: (_screenHeight / _pixelDecimation);


	// calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		_width  = width /2;
		_height = height;
		_src_x  = _cropLeft / 2;
		_src_y  = _cropTop;
		_src_x_max = (_screenWidth / 2) - _cropRight;
		_src_y_max = _screenHeight - _cropBottom;
		break;
	case VideoMode::VIDEO_3DTAB:
		_width  = width;
		_height = height / 2;
		_src_x  = _cropLeft;
		_src_y  = _cropTop / 2;
		_src_x_max = _screenWidth - _cropRight;
		_src_y_max = (_screenHeight / 2) - _cropBottom;
		break;
	case VideoMode::VIDEO_2D:
	default:
		_width  = width;
		_height = height;
		_src_x  = _cropLeft;
		_src_y  = _cropTop;
		_src_x_max = _screenWidth - _cropRight;
		_src_y_max = _screenHeight - _cropBottom;
		break;
	}

	Info(_log, "Update output image resolution to [%dx%d]", _width, _height);
	return 1;
}

void QtGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	updateScreenDimensions(true);
}

void QtGrabber::setPixelDecimation(int pixelDecimation)
{
	_pixelDecimation = pixelDecimation;
}

void QtGrabber::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	updateScreenDimensions(true);
}

void QtGrabber::setDisplayIndex(int index)
{
	if(_display != unsigned(index))
	{
		_display = unsigned(index);
		setupDisplay();
	}
}
