#pragma once

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/VideoMode.h>
#include <hyperion/GrabberWrapper.h>

// Forward class declaration
class X11Grabber;
class ImageProcessor;

///
/// The X11Wrapper uses an instance of the X11Grabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and commmited to the
/// attached Hyperion.
///
class X11Wrapper: public GrabberWrapper
{
public:
	///
	/// Constructs the framebuffer frame grabber with a specified grab size and update rate.
	///
	/// @param[in] device X11 device name/path
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	///
	X11Wrapper(bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation, const unsigned updateRate_Hz, const int priority);

	///
	/// Destructor of this framebuffer frame grabber. Releases any claimed resources.
	///
	virtual ~X11Wrapper();

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	virtual void action();

private:
	/// The actual grabber
	X11Grabber * _grabber;

	bool _init;
};
 
