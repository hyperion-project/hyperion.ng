// proj
#include <grabber/qt/QtGrabber.h>

// qt
#include <QPixmap>
#include <QWindow>
#include <QGuiApplication>
#include <QScreen>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QImage>
#include <QDebug>

#if 0 // #ifdef _WIN32 - Preserve code to handle Windows differently from standard
#include <Windows.h>
#endif

QtGrabber::QtGrabber(int display, int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GRABBER-QT", cropLeft, cropRight, cropTop, cropBottom)
	, _display(display)
	, _numberOfSDisplays(0)
	, _screenWidth(0)
	, _screenHeight(0)
	, _src_x(0)
	, _src_y(0)
	, _src_x_max(0)
	, _src_y_max(0)
	, _isWayland(false)
	, _screen(nullptr)
	, _isVirtual(false)
{
	TRACK_SCOPE() << "Creating Qt grabber for display" << _display;
	_useImageResampler = false;
	resetDeviceAndCapture();
}

QtGrabber::~QtGrabber()
{
	TRACK_SCOPE();
	freeResources();
}

void QtGrabber::freeResources()
{
	// Qt seems to hold the ownership of the QScreen pointers
}

bool QtGrabber::isAvailable(bool logError)
{
#ifndef _WIN32
	if (getenv("WAYLAND_DISPLAY") != nullptr)
	{
		ErrorIf(logError, _log, "Grabber does not work under Wayland!");
		_isWayland = true;
	}
#endif

	_isAvailable = !_isWayland;
	return _isAvailable;
}

bool QtGrabber::open()
{
	qCDebug(grabber_screen_flow) << "Opening Qt grabber for display" << _display;

	if (_isAvailable && !setupDisplay())
	{
		setInError(QString("Opening display %1 failed.").arg(_display));
		return false;
	}

	return _isAvailable;
}

bool QtGrabber::setupDisplay()
{
	if (_isDeviceInError)
	{
		Error(_log, "Cannot setup display, device is in error state");
		return false;
	}

	qCDebug(grabber_screen_flow) << "Display to setup for capture:" << _display;

	// cleanup last screen
	freeResources();

	QScreen* primary = QGuiApplication::primaryScreen();
	QList<QScreen*> screens = QGuiApplication::screens();
	// inject main screen at 0, if not nullptr
	if (primary != nullptr)
	{
		screens.prepend(primary);
		// remove last main screen if twice in list
		if (screens.lastIndexOf(primary) > 0)
		{
			screens.removeAt(screens.lastIndexOf(primary));
		}
	}

	if (screens.isEmpty())
	{
		_numberOfSDisplays = 0;
		Error(_log, "No displays found to capture from!");
		return false;
	}

	_numberOfSDisplays = screens.size();

	Info(_log, "Available Displays:");
	int index = 0;
	for (const QScreen* screen : screens)
	{
		const QRect geo = screen->geometry();
		qreal devicePixelRatio = screen->devicePixelRatio();
		Info(_log, "Display %d: Name: %s Resolution: [%dx%d], Geometry: (L,T,R,B) %d,%d,%d,%d Depth:%dbit DPR:%.2f", index, QSTRING_CSTR(screen->name()), geo.width(), geo.height(), geo.x(), geo.y(), geo.x() + geo.width(), geo.y() + geo.height(), screen->depth(), devicePixelRatio);

		++index;
	}

	if (screens.at(0)->size() != screens.at(0)->virtualSize())
	{
		const QRect vgeo = screens.at(0)->virtualGeometry();
		qreal devicePixelRatio = screens.at(0)->devicePixelRatio();
		Info(_log, "Display %d: Name: %s Resolution: [%dx%d], Geometry: (L,T,R,B) %d,%d,%d,%d Depth:%dbit DPR:%.2f", _numberOfSDisplays, "All Displays", vgeo.width(), vgeo.height(), vgeo.x(), vgeo.y(), vgeo.x() + vgeo.width(), vgeo.y() + vgeo.height(), screens.at(0)->depth(), devicePixelRatio);
	}

	_isVirtual = false;
	// be sure the index is available
	if (_display > _numberOfSDisplays - 1)
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
	connect(_screen, &QScreen::physicalDotsPerInchChanged, this, &QtGrabber::pixelRatioChanged);

	if (_isVirtual)
	{
		Info(_log, "Using virtual display across all screens");
	}
	else
	{
		Info(_log, "Initialized display %d", _display);
	}

	return true;
}

void QtGrabber::geometryChanged(const QRect& geo)
{
	Info(_log, "The current display changed geometry to (L,T,R,B) %d,%d,%d,%d", geo.left(), geo.top(), geo.x() + geo.width(), geo.y() + geo.height());
	updateScreenDimensions(true);
}

void QtGrabber::pixelRatioChanged(qreal /* dpi */)
{
	Info(_log, "The pixel ratio changed to %.2f", _screen->devicePixelRatio());
}


#if 0 // #ifdef _WIN32 - Preserve code to handle Windows differently from standard
extern QPixmap qt_pixmapFromWinHBITMAP(HBITMAP bitmap, int format = 0);

QPixmap QtGrabber::grabWindow(quintptr window, int xIn, int yIn, int width, int height) const
{
	HWND hwnd = reinterpret_cast<HWND>(window);

	QSize nativeSize(width, height);
	if (!nativeSize.isValid())
	{
		QSize windowSize;
		if (hwnd)
		{
			RECT r;
			GetClientRect(hwnd, &r);
			windowSize = QSize(r.right - r.left, r.bottom - r.top);
		}
		else
		{
			hwnd = GetDesktopWindow();
			const QRect screenGeometry = _screen->geometry();
			windowSize = screenGeometry.size();
		}

		if (width < 0)
		{
			nativeSize.setWidth(windowSize.width() - xIn);
		}

		if (height < 0)
		{
			nativeSize.setHeight(windowSize.height() - yIn);
		}
	}

	// Get device pixel ratio to handle high-DPI displays
	qreal devicePixelRatio = _screen->devicePixelRatio();

	// Scale coordinates by device pixel ratio for physical pixel accuracy
	QPoint const nativePos = QPoint(xIn, yIn) * devicePixelRatio;
	nativeSize *= devicePixelRatio;

	// Create and setup bitmap
	HDC display_dc = GetDC(nullptr);
	HDC bitmap_dc = CreateCompatibleDC(display_dc);
	HBITMAP bitmap = CreateCompatibleBitmap(display_dc, nativeSize.width(), nativeSize.height());
	HGDIOBJ null_bitmap = SelectObject(bitmap_dc, bitmap);

	// copy data
	HDC window_dc = GetDC(hwnd);
	BitBlt(bitmap_dc, 0, 0, nativeSize.width(), nativeSize.height(), window_dc, nativePos.x(), nativePos.y(), SRCCOPY);

	// clean up all but bitmap
	ReleaseDC(hwnd, window_dc);
	SelectObject(bitmap_dc, null_bitmap);
	DeleteDC(bitmap_dc);
	QPixmap pixmap = qt_pixmapFromWinHBITMAP(bitmap);
	DeleteObject(bitmap);
	ReleaseDC(nullptr, display_dc);

	// Set device pixel ratio on the pixmap to ensure proper scaling
	pixmap.setDevicePixelRatio(devicePixelRatio);

	return pixmap;
}
#endif

int QtGrabber::grabFrame(Image<ColorRgb>& image)
{
	if (_isDeviceInError)
	{
		Error(_log, "Cannot grab frame, device is in error state");
		return -1;
	}

	if (!_isEnabled)
	{
		return -1;
	}

	if (_screen == nullptr && !resetDeviceAndCapture())
	{
		Error(_log, "Failed to open or restart capture for display %d", _display);
		return -1;
	}

#if 0 // #ifdef _WIN32 - Preserve code to handle Windows differently from standard
	QPixmap originalPixmap = grabWindow(0, _src_x, _src_y, _src_x_max, _src_y_max);
#else
	QPixmap originalPixmap = _screen->grabWindow(0, _src_x, _src_y, _src_x_max, _src_y_max);
#endif

	if (originalPixmap.isNull())
	{
		return -1;
	}
	QImage imageFrame = originalPixmap.toImage().scaled(_width, _height).convertToFormat(QImage::Format_RGB888);

	// It is required that _width is a multiple of 4, so no per-row copying is needed
	unsigned char* destPtr = reinterpret_cast<unsigned char*>(image.memptr());
	memcpy(destPtr, imageFrame.constBits(), imageFrame.sizeInBytes());

	return 0;
}

int QtGrabber::updateScreenDimensions(bool force)
{

	if (_screen == nullptr)
	{
		return -1;
	}

	qCDebug(grabber_screen_flow) << "Updating screen dimensions for display" << _display << "on screen:" << _screen;

	QRect geo;

	if (_isVirtual)
	{
		geo = _screen->virtualGeometry();
	}
	else
	{
		geo = _screen->geometry();
	}
	if (!force && _screenWidth == geo.width() && _height == geo.height())
	{
		// No update required
		return 0;
	}

	// Get device pixel ratio for high-DPI display handling
	qreal devicePixelRatio = _screen->devicePixelRatio();

	Info(_log, "Update of screen resolution: [%dx%d] to [%dx%d] (Device Pixel Ratio: %.2f)", _screenWidth, _screenHeight, geo.width(), geo.height(), devicePixelRatio);
	_screenWidth = geo.width();
	_screenHeight = geo.height();

	int width = 0;
	int height = 0;

	// Image scaling is performed by Qt
	width = (_screenWidth > (_cropLeft + _cropRight))
		? ((_screenWidth - _cropLeft - _cropRight) / _pixelDecimation)
		: (_screenWidth / _pixelDecimation);

	height = (_screenHeight > (_cropTop + _cropBottom))
		? ((_screenHeight - _cropTop - _cropBottom) / _pixelDecimation)
		: (_screenHeight / _pixelDecimation);

	// calculate final image dimensions and adjust top/left cropping in 3D modes
	if (_isVirtual)
	{
		_src_x = geo.x();
		_src_y = geo.y();
	}
	else
	{
		_src_x = 0;
		_src_y = 0;
	}

	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		_width = width / 2;
		_height = height;
		_src_x = _src_x + (_cropLeft / 2);
		_src_y = _src_y + _cropTop;
		_src_x_max = (_screenWidth / 2) - _cropRight - _cropLeft;
		_src_y_max = _screenHeight - _cropBottom - _cropTop;
		break;
	case VideoMode::VIDEO_3DTAB:
		_width = width;
		_height = height / 2;
		_src_x = _src_x + _cropLeft;
		_src_y = _src_y + (_cropTop / 2);
		_src_x_max = _screenWidth - _cropRight - _cropLeft;
		_src_y_max = (_screenHeight / 2) - _cropBottom - _cropTop;
		break;
	case VideoMode::VIDEO_2D:
	default:
		_width = width;
		_height = height;
		_src_x = _src_x + _cropLeft;
		_src_y = _src_y + _cropTop;
		_src_x_max = _screenWidth - _cropRight - _cropLeft;
		_src_y_max = _screenHeight - _cropBottom - _cropTop;
		break;
	}

	// Round _width down to the nearest multiple of 4.
	// This ensures QImage::Format_RGB888 has no padding (bytesPerLine == width * 3).
	_width = (_width / 4) * 4;

	Info(_log, "Update output image resolution to [%dx%d]", _width, _height);

	// Scale coordinates by device pixel ratio to get native resolution
	QPoint const nativePos = QPoint(_src_x, _src_y) * devicePixelRatio;
	QSize const nativeSize = QSize(_src_x_max, _src_y_max) * devicePixelRatio;

	Debug(_log, "Grab screen area: %d,%d,%d,%d (Physical: %d,%d,%d,%d)", _src_x, _src_y, _src_x_max, _src_y_max,
		nativePos.x(), nativePos.y(), nativeSize.width(), nativeSize.height());

	return 1;
}

bool QtGrabber::resetDeviceAndCapture()
{
	qCDebug(grabber_screen_flow) << "Resetting device and capture for display" << _display;
	return open() && updateScreenDimensions(true);
}

void QtGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	updateScreenDimensions(true);
}

bool QtGrabber::setPixelDecimation(int pixelDecimation)
{
	if (Grabber::setPixelDecimation(pixelDecimation))
	{
		if (!setupDisplay())
		{
			return false;
		}
	}
	return true;
}

void QtGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	qCDebug(grabber_screen_flow);
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	updateScreenDimensions(true);
}

bool QtGrabber::setDisplayIndex(int index)
{
	qCDebug(grabber_screen_properties) << "setDisplayIndex:" << index << "_display" << _display;

	bool rc(true);
	if (_display != index || _isVirtual)
	{
		_isVirtual = false;
		if (index <= _numberOfSDisplays)
		{
			_display = index;
			if (index == _numberOfSDisplays)
			{
				_isVirtual = true;
			}
		}
		else
		{
			_display = 0;
		}
		rc = resetDeviceAndCapture();
	}

	qCDebug(grabber_screen_properties) << "setDisplayIndex new:" << index << "_display" << _display;
	return rc;
}

QJsonObject QtGrabber::discover(const QJsonObject& params)
{
	QJsonObject inputsDiscovered;
	if (isAvailable(false) && open())
	{
		QList<QScreen*> screens = QGuiApplication::screens();
		if (!screens.isEmpty())
		{
			inputsDiscovered["device"] = "qt";
			inputsDiscovered["device_name"] = "QT";
			inputsDiscovered["type"] = "screen";

			QJsonArray video_inputs;
			for (int i = 0; i < screens.size(); ++i)
			{
				QJsonObject in;

				QString name = screens.at(i)->name();
				qsizetype pos = name.lastIndexOf('\\');
				if (pos != -1)
				{
					name = name.right(name.length() - pos - 1);
				}

				in["name"] = name;
				in["inputIdx"] = i;

				QJsonArray formats;
				QJsonObject format;

				QJsonArray resolutionArray;

				QJsonObject resolution;

				resolution["width"] = screens.at(i)->size().width();
				resolution["height"] = screens.at(i)->size().height();
				resolution["fps"] = getFpsSupported();

				resolutionArray.append(resolution);

				format["resolutions"] = resolutionArray;
				formats.append(format);

				in["formats"] = formats;
				video_inputs.append(in);
			}

			if (screens.at(0)->size() != screens.at(0)->virtualSize())
			{
				QJsonObject input;
				input["name"] = "All Displays";
				input["inputIdx"] = screens.size();
				input["virtual"] = true;

				QJsonArray formats;
				QJsonObject format;

				QJsonArray resolutionArray;

				QJsonObject resolution;

				resolution["width"] = screens.at(0)->virtualSize().width();
				resolution["height"] = screens.at(0)->virtualSize().height();
				resolution["fps"] = getFpsSupported();

				resolutionArray.append(resolution);

				format["resolutions"] = resolutionArray;
				formats.append(format);

				input["formats"] = formats;
				video_inputs.append(input);
			}
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject resolution_default;
			resolution_default["fps"] = _fps;

			QJsonObject video_inputs_default;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;

			QJsonObject defaults;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}

		if (inputsDiscovered.isEmpty())
		{
			qCDebug(grabber_screen_properties) << "No displays found to capture from!";
		}
	}

	return inputsDiscovered;
}
