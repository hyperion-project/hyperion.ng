#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/framebuffer/FramebufferFrameGrabber.h>

///
/// The FramebufferWrapper uses an instance of the FramebufferFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and committed to the
/// attached Hyperion.
///
class FramebufferWrapper: public GrabberWrapper
{
	Q_OBJECT
public:

	static constexpr const char* GRABBERTYPE = "FB";

	///
	/// Constructs the framebuffer frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	/// @param[in] deviceIdx Framebuffer device index
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	///
	FramebufferWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
						int deviceIdx = 0,
						int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION
						);

	///
	/// Constructs the QT frame grabber from configuration settings
	///
	FramebufferWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	FramebufferFrameGrabber _grabber;
};
