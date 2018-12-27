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

	X11Grabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation);

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

	///
	/// @brief Apply new width/height values, overwrite Grabber.h implementation as X11 doesn't use width/height, just pixelDecimation to calc dimensions
	///
	virtual void setWidthHeight(int width, int height);

	///
	/// @brief Apply new pixelDecimation
	///
	virtual void setPixelDecimation(int pixelDecimation);

	///
	/// Set the crop values
	/// @param  cropLeft    Left pixel crop
	/// @param  cropRight   Right pixel crop
	/// @param  cropTop     Top pixel crop
	/// @param  cropBottom  Bottom pixel crop
	///
	virtual void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom);

private:
	bool _XShmAvailable, _XShmPixmapAvailable, _XRenderAvailable;

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
	int _pixelDecimation;

	unsigned _screenWidth;
	unsigned _screenHeight;
	unsigned _src_x;
	unsigned _src_y;

	Image<ColorRgb> _image;

	void freeResources();
	void setupResources();
};
