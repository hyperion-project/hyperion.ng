#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/DRMFrameGrabber.h>

///
/// The DRMWrapper uses an instance of the DRMFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and committed to the
/// attached Hyperion.
///
class DRMWrapper: public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the DRM frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	/// @param[in] device Framebuffer device name/path
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	///
	DRMWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ, const QString & device = "/dev/dri/card0", int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	DRMFrameGrabber _grabber;
};
