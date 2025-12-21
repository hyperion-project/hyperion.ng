#include <utils/Logger.h>
#include <grabber/x11/X11Grabber.h>

#include <xcb/randr.h>
#include <xcb/xcb_event.h>

X11Grabber::X11Grabber(int cropLeft, int cropRight, int cropTop, int cropBottom)
	: Grabber("GRABBER-X11", cropLeft, cropRight, cropTop, cropBottom)
	, _x11Display(nullptr)
	, _xImage(nullptr)
	, _pixmap(None)
	, _srcFormat(nullptr)
	, _dstFormat(nullptr)
	, _srcPicture(None)
	, _dstPicture(None)
	, _screenWidth(0)
	, _screenHeight(0)
	, _src_x(cropLeft)
	, _src_y(cropTop)
	, _xShmAvailable(false)
	, _xRenderAvailable(false)
	, _xRandRAvailable(false)
	, _isWayland (false)
{
	_useImageResampler = false;
	_imageResampler.setCropping(0, 0, 0, 0); // cropping is performed by XRender, XShmGetImage or XGetImage
	memset(&_pictAttr, 0, sizeof(_pictAttr));
	_pictAttr.repeat = RepeatNone;
}

X11Grabber::~X11Grabber()
{
	if (_x11Display != nullptr)
	{
		freeResources();
		XCloseDisplay(_x11Display);
	}
}

void X11Grabber::freeResources()
{
	// Cleanup allocated resources of the X11 grab
	if (_xImage != nullptr)
	{
		XDestroyImage(_xImage);
	}
	if (_xRandRAvailable)
	{
		qApp->removeNativeEventFilter(this);
	}
	if(_xShmAvailable)
	{
		XShmDetach(_x11Display, &_shminfo);
		shmdt(_shminfo.shmaddr);
		shmctl(_shminfo.shmid, IPC_RMID, nullptr);
	}
	if (_xRenderAvailable)
	{
		XRenderFreePicture(_x11Display, _srcPicture);
		XRenderFreePicture(_x11Display, _dstPicture);
		XFreePixmap(_x11Display, _pixmap);
	}
}

void X11Grabber::setupResources()
{
	if (_xRandRAvailable)
	{
		qApp->installNativeEventFilter(this);
	}

	if(_xShmAvailable)
	{
		_xImage = XShmCreateImage(_x11Display, _windowAttr.visual, _windowAttr.depth, ZPixmap, nullptr, &_shminfo, _width, _height);
		_shminfo.shmid = shmget(IPC_PRIVATE, (size_t) _xImage->bytes_per_line * _xImage->height, IPC_CREAT|0777);
		_xImage->data = (char*)shmat(_shminfo.shmid,nullptr,0);
		_shminfo.shmaddr = _xImage->data;
		_shminfo.readOnly = False;
		XShmAttach(_x11Display, &_shminfo);
	}

	if (_xRenderAvailable)
	{
		_useImageResampler = false;
		_imageResampler.setHorizontalPixelDecimation(1);
		_imageResampler.setVerticalPixelDecimation(1);

		if(_xShmPixmapAvailable)
		{
			_pixmap = XShmCreatePixmap(_x11Display, _window, _xImage->data, &_shminfo, _width, _height, _windowAttr.depth);
		}
		else
		{
			_pixmap = XCreatePixmap(_x11Display, _window, _width, _height, _windowAttr.depth);
		}
		_srcFormat = XRenderFindVisualFormat(_x11Display, _windowAttr.visual);
		_dstFormat = XRenderFindVisualFormat(_x11Display, _windowAttr.visual);
		_srcPicture = XRenderCreatePicture(_x11Display, _window, _srcFormat, CPRepeat, &_pictAttr);
		_dstPicture = XRenderCreatePicture(_x11Display, _pixmap, _dstFormat, CPRepeat, &_pictAttr);

		XRenderSetPictureFilter(_x11Display, _srcPicture, FilterBilinear, nullptr, 0);
	}
	else
	{
		_useImageResampler = true;
		_imageResampler.setHorizontalPixelDecimation(_pixelDecimation);
		_imageResampler.setVerticalPixelDecimation(_pixelDecimation);
	}
}


bool X11Grabber::isAvailable(bool logError)
{
	if (getenv("WAYLAND_DISPLAY") != nullptr)
	{
		ErrorIf(logError, _log, "Grabber does not work under Wayland!");
		_isWayland = true;
	}

	_isAvailable = !_isWayland;
	return _isAvailable;
}


bool X11Grabber::open()
{
	bool rc = false;

	if (_isAvailable)
	{
		_x11Display = XOpenDisplay(nullptr);
		if (_x11Display != nullptr)
		{
			rc = true;
		}
	}
	return rc;
}

bool X11Grabber::setupDisplay()
{
	if (!_isAvailable)
	{
		return false;
	}

	bool result = false;

	if ( ! open() )
	{
		if (getenv("DISPLAY") != nullptr)
		{
			Error(_log, "Unable to open display [%s]",getenv("DISPLAY"));
		}
		else
		{
			Error(_log, "DISPLAY environment variable not set");
		}

	}
	else
	{
		_window = DefaultRootWindow(_x11Display);

		int dummy;
		int pixmaps_supported;

		_xRandRAvailable = (XRRQueryExtension(_x11Display, &_xRandREventBase, &dummy) != 0);
		_xRenderAvailable = (XRenderQueryExtension(_x11Display, &dummy, &dummy) != 0);
		_xShmAvailable = (XShmQueryExtension(_x11Display) != 0);
		XShmQueryVersion(_x11Display, &dummy, &dummy, &pixmaps_supported);
		_xShmPixmapAvailable = (pixmaps_supported != 0) && XShmPixmapFormat(_x11Display) == ZPixmap;

		Info(_log, "%s", QSTRING_CSTR(QString("XRandR=[%1] XRender=[%2] XShm=[%3] XPixmap=[%4]")
			 .arg(_xRandRAvailable     ? "available" : "unavailable",
			 _xRenderAvailable    ? "available" : "unavailable",
			 _xShmAvailable       ? "available" : "unavailable",
			 _xShmPixmapAvailable ? "available" : "unavailable"))
			 );

		result = (updateScreenDimensions(true) >=0);
		ErrorIf(!result, _log, "X11 Grabber start failed");
		setEnabled(result);
	}
	return result;
}

int X11Grabber::grabFrame(Image<ColorRgb> & image, bool forceUpdate)
{
	if (!_isEnabled)
	{
		return 0;
	}

	if (forceUpdate)
	{
		updateScreenDimensions(forceUpdate);
	}

	if (_xRenderAvailable)
	{
		double scale_x = static_cast<double>(_windowAttr.width / _pixelDecimation) / static_cast<double>(_windowAttr.width);
		double scale_y = static_cast<double>(_windowAttr.height / _pixelDecimation) / static_cast<double>(_windowAttr.height);
		double scale = qMin(scale_y, scale_x);

		_transform =
		{
			{
				{
					XDoubleToFixed(1),
					XDoubleToFixed(0),
					XDoubleToFixed(0)
				},
				{
					XDoubleToFixed(0),
					XDoubleToFixed(1),
					XDoubleToFixed(0)
				},
				{
					XDoubleToFixed(0),
					XDoubleToFixed(0),
					XDoubleToFixed(scale)
				}
			}
		};

		XRenderSetPictureTransform (_x11Display, _srcPicture, &_transform);

		// display, op, src, mask, dest, src_x = cropLeft,
		// src_y = cropTop, mask_x, mask_y, dest_x, dest_y, width, height
		XRenderComposite(
			_x11Display, PictOpSrc, _srcPicture, None, _dstPicture, ( _src_x/_pixelDecimation),
			(_src_y/_pixelDecimation), 0, 0, 0, 0, _width, _height);

		XSync(_x11Display, False);

		if (_xShmAvailable)
		{
			XShmGetImage(_x11Display, _pixmap, _xImage, 0, 0, AllPlanes);
		}
		else
		{
			_xImage = XGetImage(_x11Display, _pixmap, 0, 0, _width, _height, AllPlanes, ZPixmap);
		}
	}
	else if (_xShmAvailable)
	{
		// use xshm
		XShmGetImage(_x11Display, _window, _xImage, _src_x, _src_y, AllPlanes);
	}
	else
	{
		// all things done by xgetimage
		_xImage = XGetImage(_x11Display, _window, _src_x, _src_y, _width, _height, AllPlanes, ZPixmap);
	}

	if (_xImage == nullptr)
	{
		Error(_log, "Grab Failed!");
		return -1;
	}

	_imageResampler.processImage(reinterpret_cast<const uint8_t *>(_xImage->data), _xImage->width, _xImage->height, _xImage->bytes_per_line, PixelFormat::BGR32, image);

	return 0;
}

int X11Grabber::updateScreenDimensions(bool force)
{
	const Status status = XGetWindowAttributes(_x11Display, _window, &_windowAttr);
	if (status == 0)
	{
		Error(_log, "Failed to obtain window attributes");
		return -1;
	}

	if (!force && _screenWidth == _windowAttr.width && _screenHeight == _windowAttr.height)
	{
		// No update required
		return 0;
	}

	if ((_screenWidth != 0) || (_screenHeight != 0))
	{
		freeResources();
	}

	Info(_log, "Update of screen resolution: [%dx%d]  to [%dx%d]", _screenWidth, _screenHeight, _windowAttr.width, _windowAttr.height);
	_screenWidth  = _windowAttr.width;
	_screenHeight = _windowAttr.height;

	int width=0;
	int height=0;

	// Image scaling is performed by XRender when available, otherwise by ImageResampler
	if (_xRenderAvailable)
	{
		width  =  (_screenWidth > (_cropLeft + _cropRight))
			? ((_screenWidth - _cropLeft - _cropRight) / _pixelDecimation)
			: _screenWidth / _pixelDecimation;

		height =  (_screenHeight > (_cropTop + _cropBottom))
			? ((_screenHeight - _cropTop - _cropBottom) / _pixelDecimation)
			: _screenHeight / _pixelDecimation;

		Info(_log, "Using XRender for grabbing");
	}
	else
	{
		width  =  (_screenWidth > (_cropLeft + _cropRight))
			? (_screenWidth - _cropLeft - _cropRight)
			: _screenWidth;

		height =  (_screenHeight > (_cropTop + _cropBottom))
			? (_screenHeight - _cropTop - _cropBottom)
			: _screenHeight;

		Info(_log, "Using XGetImage for grabbing");
	}

	// calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
	case VideoMode::VIDEO_3DSBS:
		_width  = width /2;
		_height = height;
		_src_x  = _cropLeft / 2;
		_src_y  = _cropTop;
		break;
	case VideoMode::VIDEO_3DTAB:
		_width  = width;
		_height = height / 2;
		_src_x  = _cropLeft;
		_src_y  = _cropTop / 2;
		break;
	case VideoMode::VIDEO_2D:
	default:
		_width  = width;
		_height = height;
		_src_x  = _cropLeft;
		_src_y  = _cropTop;
		break;
	}

	Info(_log, "Update output image resolution to [%dx%d]", _width, _height);
	_image.resize(_width, _height);
	setupResources();

	return 1;
}

void X11Grabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	if(_x11Display != nullptr)
	{
		updateScreenDimensions(true);
	}
}

bool X11Grabber::setPixelDecimation(int pixelDecimation)
{
	bool rc (true);
	if (Grabber::setPixelDecimation(pixelDecimation))
	{
		if(_x11Display != nullptr)
		{
			if ( updateScreenDimensions(true) < 0 )
			{
				rc = false;
			}
		}
	}
	return rc;
}

void X11Grabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	if(_x11Display != nullptr)
	{
		updateScreenDimensions(true); // segfault on init
	}
}

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
bool X11Grabber::nativeEventFilter(const QByteArray & eventType, void * message, qintptr * /*result*/)
#else
bool X11Grabber::nativeEventFilter(const QByteArray & eventType, void * message, long int * /*result*/)
#endif
{
	if (!_xRandRAvailable || eventType != "xcb_generic_event_t") {
		return false;
	}

	auto event = static_cast<xcb_generic_event_t*>(message);
	const uint8_t xEventType = XCB_EVENT_RESPONSE_TYPE(event);

	if (xEventType == _xRandREventBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
	{
		updateScreenDimensions(true);
	}

	return false;
}

QJsonObject X11Grabber::discover(const QJsonObject& params)
{
	QJsonObject inputsDiscovered;
	if ( isAvailable(false) && open() )
	{
		inputsDiscovered["device"] = "x11";
		inputsDiscovered["device_name"] = "X11";
		inputsDiscovered["type"] = "screen";

		QJsonArray video_inputs;

		if (_x11Display != nullptr)
		{
			QJsonArray fps = { 1, 5, 10, 15, 20, 25, 30, 40, 50, 60 };

			// Iterate through all X screens
			for (int i = 0; i < XScreenCount(_x11Display); ++i)
			{
				_window = DefaultRootWindow(_x11Display);

				const Status status = XGetWindowAttributes(_x11Display, _window, &_windowAttr);
				if (status == 0)
				{
					Debug(_log, "Failed to obtain window attributes");
				}
				else
				{
					QJsonObject input;

					QString displayName;
					char* name;
					if ( XFetchName(_x11Display, _window, &name) > 0 )
					{
						 displayName = name;
					}
					else {
						displayName = QString("Display:%1").arg(i);
					}

					input["name"] = displayName;
					input["inputIdx"] = i;

					QJsonArray formats;
					QJsonArray resolutionArray;
					QJsonObject format;
					QJsonObject resolution;

					resolution["width"] = _windowAttr.width;
					resolution["height"] = _windowAttr.height;
					resolution["fps"] = fps;

					resolutionArray.append(resolution);

					format["resolutions"] = resolutionArray;
					formats.append(format);

					input["formats"] = formats;
					video_inputs.append(input);
				}
			}

			if ( !video_inputs.isEmpty() )
			{
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
		}
	}

	return inputsDiscovered;
}

