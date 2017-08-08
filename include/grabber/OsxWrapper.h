#pragma once

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/GrabbingMode.h>
#include <utils/VideoMode.h>
#include <hyperion/GrabberWrapper.h>

// Forward class declaration
class OsxFrameGrabber;
class ImageProcessor;

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
	/// @param[in] hyperion  The instance of Hyperion used to write the led values
	///
	OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority);

	///
	/// Destructor of this osx frame grabber. Releases any claimed resources.
	///
	virtual ~OsxWrapper();

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	virtual void action();

private:
	/// The actual grabber
	OsxFrameGrabber * _grabber;

	/// The list with computed led colors
	std::vector<ColorRgb> _ledColors;
};
