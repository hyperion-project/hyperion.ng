#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/OsxFrameGrabber.h>

///
/// The OsxWrapper uses an instance of the OsxFrameGrabber to obtain ImageRgb's from the displayed content.
/// This ImageRgb is processed to a ColorRgb for each led and committed to the attached Hyperion.
///
class OsxWrapper: public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the osx frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	/// @param[in] display Index of the display to grab
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	///
	OsxWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
				int display = kCGDirectMainDisplay,
				int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION
				);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	OsxFrameGrabber _grabber;
};
