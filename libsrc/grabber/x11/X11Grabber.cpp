// STL includes
#include <iostream>
#include <cstdint>

// X11Grabber includes
#include <grabber/X11Grabber.h>

X11Grabber::X11Grabber(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation) :
	_imageResampler(),
	_useXGetImage(useXGetImage),
	_cropLeft(cropLeft),
	_cropRight(cropRight),
	_cropTop(cropTop),
	_cropBottom(cropBottom),
	_x11Display(nullptr),
	_pixmap(None),
	_srcFormat(nullptr),
	_dstFormat(nullptr),
	_srcPicture(None),
	_dstPicture(None),
	_screenWidth(0),
	_screenHeight(0),
	_croppedWidth(0),
	_croppedHeight(0),
	_image(0,0)
{
	_imageResampler.setHorizontalPixelDecimation(horizontalPixelDecimation);
	_imageResampler.setVerticalPixelDecimation(verticalPixelDecimation);
	_imageResampler.setCropping(0, 0, 0, 0); // cropping is performed by XShmGetImage or XGetImage
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

void X11Grabber::setVideoMode(const VideoMode videoMode)
{
	_imageResampler.set3D(videoMode);
}

void X11Grabber::freeResources()
{
	// Cleanup allocated resources of the X11 grab
	XDestroyImage(_xImage);
	if(_XShmAvailable && !_useXGetImage)   {
		XShmDetach(_x11Display, &_shminfo);
		shmdt(_shminfo.shmaddr);
		shmctl(_shminfo.shmid, IPC_RMID, 0);
	}
	if (_XRenderAvailable && !_useXGetImage) {
		XRenderFreePicture(_x11Display, _srcPicture);
		XRenderFreePicture(_x11Display, _dstPicture);
		XFreePixmap(_x11Display, _pixmap);
	}
}

void X11Grabber::setupResources()
{
	if(_XShmAvailable && !_useXGetImage) {
		_xImage = XShmCreateImage(_x11Display, _windowAttr.visual,
		_windowAttr.depth, ZPixmap, NULL, &_shminfo,
		_croppedWidth, _croppedHeight);

		_shminfo.shmid = shmget(IPC_PRIVATE, _xImage->bytes_per_line * _xImage->height, IPC_CREAT|0777);
		_xImage->data = (char*)shmat(_shminfo.shmid,0,0);
		_shminfo.shmaddr = _xImage->data;
		_shminfo.readOnly = False;
	
	XShmAttach(_x11Display, &_shminfo);
	}
	if (_XRenderAvailable && !_useXGetImage) {
		if(_XShmPixmapAvailable) {
			_pixmap = XShmCreatePixmap(_x11Display, _window, _xImage->data, &_shminfo, _croppedWidth, _croppedHeight, _windowAttr.depth);
		} else {
			_pixmap = XCreatePixmap(_x11Display, _window, _croppedWidth, _croppedHeight, _windowAttr.depth);
		}
		_srcFormat = XRenderFindVisualFormat(_x11Display, _windowAttr.visual);
		_dstFormat = XRenderFindVisualFormat(_x11Display, _windowAttr.visual);
		_srcPicture = XRenderCreatePicture(_x11Display, _window, _srcFormat, CPRepeat, &_pictAttr);
		_dstPicture = XRenderCreatePicture(_x11Display, _pixmap, _dstFormat, CPRepeat, &_pictAttr);
		XRenderSetPictureFilter(_x11Display, _srcPicture, "bilinear", NULL, 0);
	}
}

bool X11Grabber::Setup()
{
	_x11Display = XOpenDisplay(NULL);
	if (_x11Display == nullptr)
	{
		std::cerr << "X11GRABBER ERROR: Unable to open display";
		if (getenv("DISPLAY")) {
			std::cerr <<  " " << std::string(getenv("DISPLAY")) << std::endl;
		} else {
			std::cerr << ". DISPLAY environment variable not set" << std::endl;
		}
		return false;
	}
	 
	_window = DefaultRootWindow(_x11Display);

	int dummy, pixmaps_supported;
	
	_XRenderAvailable = XRenderQueryExtension(_x11Display, &dummy, &dummy);
	_XShmAvailable = XShmQueryExtension(_x11Display);
	XShmQueryVersion(_x11Display, &dummy, &dummy, &pixmaps_supported);
	_XShmPixmapAvailable = pixmaps_supported && XShmPixmapFormat(_x11Display) == ZPixmap;

	return true;
}

Image<ColorRgb> & X11Grabber::grab()
{
	updateScreenDimensions(); 

	if (_XRenderAvailable && !_useXGetImage) {
		XRenderComposite( _x11Display,		// *dpy,
							PictOpSrc,		// op,
							_srcPicture,		// src
							None,			// mask
							_dstPicture,		// dst
							_cropLeft,		// src_x
							_cropTop,		// src_y
							0,			// mask_x
							0,			// mask_y
							0,			// dst_x
							0,			// dst_y
								_croppedWidth,	// width
							_croppedHeight);	// height
	
		XSync(_x11Display, False);

		if (_XShmAvailable) {
			XShmGetImage(_x11Display, _pixmap, _xImage, 0, 0, AllPlanes);
		} else {
			_xImage = XGetImage(_x11Display, _pixmap, 0, 0, _croppedWidth, _croppedHeight, AllPlanes, ZPixmap);   
		}
	} else {
		if (_XShmAvailable && !_useXGetImage) {
			XShmGetImage(_x11Display, _window, _xImage, _cropLeft, _cropTop, AllPlanes);
		} else {
			_xImage = XGetImage(_x11Display, _window, _cropLeft, _cropTop, _croppedWidth, _croppedHeight, AllPlanes, ZPixmap);
		}
	}

	if (_xImage == nullptr)
	{
		std::cerr << "X11GRABBER ERROR: Grab failed" << std::endl;
		return _image;
	}

	_imageResampler.processImage(reinterpret_cast<const uint8_t *>(_xImage->data), _xImage->width, _xImage->height, _xImage->bytes_per_line, PIXELFORMAT_BGR32, _image);

	return _image;
}

int X11Grabber::updateScreenDimensions()
{
	const Status status = XGetWindowAttributes(_x11Display, _window, &_windowAttr);
	if (status == 0)
	{
		std::cerr << "X11GRABBER ERROR: Failed to obtain window attributes" << std::endl;
		return -1;
	}

	if (_screenWidth == unsigned(_windowAttr.width) && _screenHeight == unsigned(_windowAttr.height))
	{
		// No update required
		return 0;
	}
	
	std::cout << "X11GRABBER INFO: Update of screen resolution: [" << _screenWidth << "x" << _screenHeight <<"] => ";

	if (_screenWidth || _screenHeight) {
		freeResources();
	}

	_screenWidth  = _windowAttr.width;
	_screenHeight = _windowAttr.height;
	
	std::cout << "[" << _screenWidth << "x" << _screenHeight <<"]" << std::endl;
	
	_croppedWidth  =  (_screenWidth > unsigned(_cropLeft + _cropRight))
		? (_screenWidth - _cropLeft - _cropRight)
		: _screenWidth;
	
	_croppedHeight =  (_screenHeight > unsigned(_cropTop + _cropBottom))
		? (_screenHeight - _cropTop - _cropBottom)
		: _screenHeight;

	std::cout << "X11GRABBER INFO: Using ";

	if (_XRenderAvailable && !_useXGetImage) {
		std::cout << "XRender for grabbing" << std::endl;
	} else {
		std::cout << "XGetImage for grabbing" << std::endl;
	}

	setupResources();

	return 0;
}
