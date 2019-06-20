#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/AmlogicGrabber.h>

///
/// The DispmanxWrapper uses an instance of the DispmanxFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and commmited to the
/// attached Hyperion.
///
class AmlogicWrapper : public GrabberWrapper
{
	Q_OBJECT
public:
	///
	/// Constructs the dispmanx frame grabber with a specified grab size and update rate.
	///
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	///
	AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight);

	///
	/// Destructor of this dispmanx frame grabber. Releases any claimed resources.
	///
	virtual ~AmlogicWrapper() {};

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	virtual void action();

private:
	/// The actual grabber
	AmlogicGrabber  _grabber;
};
