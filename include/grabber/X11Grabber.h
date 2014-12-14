
// Hyperion-utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/ImageResampler.h>

// X11 includes
#include <X11/Xlib.h>

class X11Grabber
{
public:

    X11Grabber(int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation);

	virtual ~X11Grabber();

	int open();

	Image<ColorRgb> & grab();

private:
    ImageResampler _imageResampler;

    int _cropLeft;
    int _cropRight;
    int _cropTop;
    int _cropBottom;

	/// Reference to the X11 display (nullptr if not opened)
	Display * _x11Display;

	unsigned _screenWidth;
	unsigned _screenHeight;

	Image<ColorRgb> _image;

	int updateScreenDimensions();
};
