#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/QtGrabber.h>

///
/// The QtWrapper uses QtFramework API's to get a picture from system
///
class QtWrapper: public GrabberWrapper
{
public:
	///
	/// Constructs the framebuffer frame grabber with a specified grab size and update rate.
	///
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	///
	QtWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display, unsigned updateRate_Hz);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	QtGrabber _grabber;
};
