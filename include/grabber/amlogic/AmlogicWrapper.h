#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/amlogic/AmlogicGrabber.h>

///
/// The Amlogic uses an instance of the AmlogicGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and committed to the
/// attached Hyperion.
///
class AmlogicWrapper : public GrabberWrapper
{
	Q_OBJECT
public:

	static constexpr const char* GRABBERTYPE = "Amlogic";

	///
	/// Constructs the Amlogic frame grabber
	///
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] deviceIdx Framebuffer device index	
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	///
	explicit AmlogicWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
							int deviceIdx = 0,
							int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
							int cropLeft = 0, int cropRight = 0,
							int cropTop = 0, int cropBottom = 0);

	///
	/// Constructs the Amlogic frame grabber from configuration settings
	///
	explicit AmlogicWrapper(const QJsonDocument& grabberConfig = QJsonDocument());


public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	AmlogicGrabber  _grabber;
};
