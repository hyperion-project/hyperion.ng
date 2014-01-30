
// Hyperion-utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

// X11 includes
#include <X11/Xlib.h>

class X11Grabber
{
public:

	X11Grabber(const unsigned cropHorizontal, const unsigned cropVertical, const unsigned pixelDecimation);

	virtual ~X11Grabber();

	int open();

	Image<ColorRgb> & grab();

private:

	const unsigned _pixelDecimation;

	const unsigned _cropWidth;
	const unsigned _cropHeight;

	/// Reference to the X11 display (nullptr if not opened)
	Display * _x11Display;

	unsigned _screenWidth;
	unsigned _screenHeight;

	Image<ColorRgb> _image;

	int updateScreenDimensions();
};
