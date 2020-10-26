#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/DirectXGrabber.h>

class DirectXWrapper: public GrabberWrapper
{
public:
	///
	/// Constructs the DirectX grabber with a specified grab size and update rate.
	///
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] display           The display used[index]
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	///
	DirectXWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, int display, const unsigned updateRate_Hz);

	///
	/// Destructor of this DirectX grabber. Releases any claimed resources.
	///
	virtual ~DirectXWrapper() {};

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	virtual void action();

private:
	/// The actual grabber
	DirectXGrabber _grabber;
};
