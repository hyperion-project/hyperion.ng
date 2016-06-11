
// Hyperion-utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/ImageResampler.h>

// X11 includes
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <X11/extensions/XShm.h>
#include <sys/ipc.h>
#include <sys/shm.h>

class X11Grabber
{
public:

	X11Grabber(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation);

	virtual ~X11Grabber();

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(const VideoMode videoMode);
	
	bool Setup();

	Image<ColorRgb> & grab();

private:
	ImageResampler _imageResampler;
	
	bool _useXGetImage, _XShmAvailable, _XShmPixmapAvailable, _XRenderAvailable;
	int _cropLeft;
	int _cropRight;
	int _cropTop;
	int _cropBottom;
	
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

	unsigned _screenWidth;
	unsigned _screenHeight;
	unsigned _croppedWidth;
	unsigned _croppedHeight;

	Image<ColorRgb> _image;
	
	void freeResources();
	void setupResources();
	
	int updateScreenDimensions();
};
