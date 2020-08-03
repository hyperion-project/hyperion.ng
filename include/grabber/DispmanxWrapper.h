#pragma once

// Utils includes
#include <utils/ColorRgba.h>
#include <hyperion/GrabberWrapper.h>
#include <grabber/DispmanxFrameGrabber.h>

///
/// The DispmanxWrapper uses an instance of the DispmanxFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is forwarded to all Hyperion instances via HyperionDaemon
///
class DispmanxWrapper: public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the dispmanx frame grabber with a specified grab size and update rate.
	///
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	///
	DispmanxWrapper(unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	DispmanxFrameGrabber _grabber;
};
