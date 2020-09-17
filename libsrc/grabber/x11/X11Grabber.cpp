#include <utils/Logger.h>
#include <grabber/X11Grabber.h>

#include <xcb/randr.h>
#include <xcb/xcb_event.h>

X11Grabber::X11Grabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation)
	: Grabber("X11GRABBER", 0, 0, cropLeft, cropRight, cropTop, cropBottom)
	, _x11Display(nullptr)
	, _pixmap(None)
	, _srcFormat(nullptr)
	, _dstFormat(nullptr)
	, _srcPicture(None)
	, _dstPicture(None)
	, _pixelDecimation(pixelDecimation)
	, _screenWidth(0)
	, _screenHeight(0)
	, _src_x(cropLeft)
	, _src_y(cropTop)
	, _image(0,0)
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
	XDestroyImage(_xImage);
	if (_XRandRAvailable)
	{
		qApp->removeNativeEventFilter(this);
	}
	if(_XShmAvailable)
	{
		XShmDetach(_x11Display, &_shminfo);
		shmdt(_shminfo.shmaddr);
		shmctl(_shminfo.shmid, IPC_RMID, 0);
	}
	if (_XRenderAvailable)
	{
		XRenderFreePicture(_x11Display, _srcPicture);
		XRenderFreePicture(_x11Display, _dstPicture);
		XFreePixmap(_x11Display, _pixmap);
	}
}

void X11Grabber::setupResources()
{
	if (_XRandRAvailable)
	{
		qApp->installNativeEventFilter(this);
	}

	if(_XShmAvailable)
	{
		_xImage = XShmCreateImage(_x11Display, _windowAttr.visual, _windowAttr.depth, ZPixmap, NULL, &_shminfo, _width, _height);
		_shminfo.shmid = shmget(IPC_PRIVATE, (size_t) _xImage->bytes_per_line * _xImage->height, IPC_CREAT|0777);
		_xImage->data = (char*)shmat(_shminfo.shmid,0,0);
		_shminfo.shmaddr = _xImage->data;
		_shminfo.readOnly = False;
		XShmAttach(_x11Display, &_shminfo);
	}

	if (_XRenderAvailable)
	{
        	_useImageResampler = false;
		_imageResampler.setHorizontalPixelDecimation(1);
		_imageResampler.setVerticalPixelDecimation(1);

		if(_XShmPixmapAvailable)
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

		XRenderSetPictureFilter(_x11Display, _srcPicture, FilterBilinear, NULL, 0);
	}
	else
	{
        	_useImageResampler = true;
		_imageResampler.setHorizontalPixelDecimation(_pixelDecimation);
		_imageResampler.setVerticalPixelDecimation(_pixelDecimation);
	}

}

bool X11Grabber::Setup()
{
	_x11Display = XOpenDisplay(NULL);
	if (_x11Display == nullptr)
	{
		Error(_log, "Unable to open display");
		if (getenv("DISPLAY"))
		{
			Error(_log, "%s",getenv("DISPLAY"));
		}
		else
		{
			Error(_log, "DISPLAY environment variable not set");
		}
		return false;
	}

	_window = DefaultRootWindow(_x11Display);

	int dummy, pixmaps_supported;

	_XRandRAvailable = XRRQueryExtension(_x11Display, &_XRandREventBase, &dummy);
	_XRenderAvailable = XRenderQueryExtension(_x11Display, &dummy, &dummy);
	_XShmAvailable = XShmQueryExtension(_x11Display);
	XShmQueryVersion(_x11Display, &dummy, &dummy, &pixmaps_supported);
	_XShmPixmapAvailable = pixmaps_supported && XShmPixmapFormat(_x11Display) == ZPixmap;

	bool result = (updateScreenDimensions(true) >=0);
	ErrorIf(!result, _log, "X11 Grabber start failed");
	setEnabled(result);
	return result;
}

int X11Grabber::grabFrame(Image<ColorRgb> & image, bool forceUpdate)
{
	if (!_enabled) return 0;

	if (forceUpdate)
		updateScreenDimensions(forceUpdate);

	if (_XRenderAvailable)
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

		if (_XShmAvailable)
		{
			XShmGetImage(_x11Display, _pixmap, _xImage, 0, 0, AllPlanes);
		}
		else
		{
			_xImage = XGetImage(_x11Display, _pixmap, 0, 0, _width, _height, AllPlanes, ZPixmap);
		}
	}
	else if (_XShmAvailable)
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

	if (!force && _screenWidth == unsigned(_windowAttr.width) && _screenHeight == unsigned(_windowAttr.height))
	{
		// No update required
		return 0;
	}

	if (_screenWidth || _screenHeight)
	{
		freeResources();
	}

	Info(_log, "Update of screen resolution: [%dx%d]  to [%dx%d]", _screenWidth, _screenHeight, _windowAttr.width, _windowAttr.height);
	_screenWidth  = _windowAttr.width;
	_screenHeight = _windowAttr.height;

	int width=0, height=0;

	// Image scaling is performed by XRender when available, otherwise by ImageResampler
	if (_XRenderAvailable)
	{
		width  =  (_screenWidth > unsigned(_cropLeft + _cropRight))
			? ((_screenWidth - _cropLeft - _cropRight) / _pixelDecimation)
			: _screenWidth / _pixelDecimation;

		height =  (_screenHeight > unsigned(_cropTop + _cropBottom))
			? ((_screenHeight - _cropTop - _cropBottom) / _pixelDecimation)
			: _screenHeight / _pixelDecimation;

		Info(_log, "Using XRender for grabbing");
	}
	else
	{
		width  =  (_screenWidth > unsigned(_cropLeft + _cropRight))
			? (_screenWidth - _cropLeft - _cropRight)
			: _screenWidth;

		height =  (_screenHeight > unsigned(_cropTop + _cropBottom))
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

	Info(_log, "Update output image resolution: [%dx%d]  to [%dx%d]", _image.width(), _image.height(), _width, _height);

	_image.resize(_width, _height);
	setupResources();

	return 1;
}

void X11Grabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	updateScreenDimensions(true);
}

void X11Grabber::setPixelDecimation(int pixelDecimation)
{
	if(_pixelDecimation != pixelDecimation)
	{
		_pixelDecimation = pixelDecimation;
		updateScreenDimensions(true);
	}
}

void X11Grabber::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	if(_x11Display != nullptr) updateScreenDimensions(true); // segfault on init
}

bool X11Grabber::nativeEventFilter(const QByteArray & eventType, void * message, long int * /*result*/)
{
	if (!_XRandRAvailable || eventType != "xcb_generic_event_t") {
		return false;
	}

	xcb_generic_event_t *e = static_cast<xcb_generic_event_t*>(message);
	const uint8_t xEventType = XCB_EVENT_RESPONSE_TYPE(e);

	if (xEventType == _XRandREventBase + XCB_RANDR_SCREEN_CHANGE_NOTIFY)
	{
		updateScreenDimensions(true);
	}

	return false;
}
