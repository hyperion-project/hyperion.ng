#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/OsxFrameGrabber.h>

///
/// The OsxWrapper uses an instance of the OsxFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and commmited to the
/// attached Hyperion.
///
class OsxWrapper: public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the osx frame grabber with a specified grab size and update rate.
	///
	/// @param[in] display Index of the display to grab
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	///
	OsxWrapper(unsigned display, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz);

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	OsxFrameGrabber _grabber;
};
