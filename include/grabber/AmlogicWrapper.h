#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/AmlogicGrabber.h>

///
/// The Amlogic uses an instance of the AmlogicGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and committed to the
/// attached Hyperion.
///
class AmlogicWrapper : public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the Amlogic frame grabber
	///
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	///
	AmlogicWrapper(int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
					int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	AmlogicGrabber  _grabber;
};
