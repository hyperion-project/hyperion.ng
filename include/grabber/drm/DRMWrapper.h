#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/drm/DRMFrameGrabber.h>

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
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] device Framebuffer device name/path
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	///
	DRMWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
			    int deviceIdx = 0,
			    int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
			    int cropLeft=0, int cropRight=0,
			    int cropTop=0, int cropBottom=0
			   );
	///
	/// Constructs the QT frame grabber from configuration settings
	///
	DRMWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	DRMFrameGrabber _grabber;
};
