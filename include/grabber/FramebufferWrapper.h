#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/FramebufferFrameGrabber.h>

///
/// The FramebufferWrapper uses an instance of the FramebufferFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and commmited to the
/// attached Hyperion.
///
class FramebufferWrapper: public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the framebuffer frame grabber with a specified grab size and update rate.
	///
	/// @param[in] device Framebuffer device name/path
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	///
	FramebufferWrapper(const QString & device, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	FramebufferFrameGrabber _grabber;
};
