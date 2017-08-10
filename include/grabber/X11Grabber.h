#pragma once

#include <QObject>

// Hyperion-utils includes
#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

// X11 includes
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

class X11Grabber : public Grabber
{
public:

	X11Grabber(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation);

	virtual ~X11Grabber();

	bool Setup();
	
	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	virtual int grabFrame(Image<ColorRgb> & image, bool forceUpdate=false);
	
	///
	/// update dimension according current screen
	int updateScreenDimensions(bool force=false);

	virtual void setVideoMode(VideoMode mode);

private:
	bool _useXGetImage, _XShmAvailable, _XShmPixmapAvailable, _XRenderAvailable;

	XImage* _xImage;
	XShmSegmentInfo _shminfo;

	/// Reference to the X11 display (nullptr if not opened)
	Display* _x11Display;
	Window _window;
	XWindowAttributes _windowAttr;
	
	Pixmap _pixmap;
	XRenderPictFormat* _srcFormat;
	XRenderPictFormat* _dstFormat;
	XRenderPictureAttributes _pictAttr;
	Picture _srcPicture;
	Picture _dstPicture;
	
	XTransform _transform;
	int _horizontalDecimation;
	int _verticalDecimation;

	unsigned _screenWidth;
	unsigned _screenHeight;
	unsigned _src_x;
	unsigned _src_y;

	Image<ColorRgb> _image;
	
	void freeResources();
	void setupResources();
};
