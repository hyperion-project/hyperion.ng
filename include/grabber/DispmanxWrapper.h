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
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	///
	DispmanxWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
					 int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION
					 );

	bool screenInit();

	bool available = false;

	///
	/// Starts the grabber which produces led values with the specified update rate
	///
	bool open() override;

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	DispmanxFrameGrabber _grabber;
};
