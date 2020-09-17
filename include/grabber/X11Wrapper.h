#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/X11Grabber.h>
// some include of xorg defines "None" this is also used by QT and has to be undefined to avoid collisions
#ifdef None
	#undef None
#endif


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
	X11Wrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, unsigned updateRate_Hz);

	///
	/// Destructor of this framebuffer frame grabber. Releases any claimed resources.
	///
	~X11Wrapper() override;

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	X11Grabber _grabber;

	bool _init;
};
