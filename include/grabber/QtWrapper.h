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
	/// Constructs the QT frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] display           Display to be grabbed
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]

	///
	QtWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
			   int display=0,
			   int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
			   int cropLeft=0, int cropRight=0,
			   int cropTop=0, int cropBottom=0
			   );

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
	QtGrabber _grabber;
};
