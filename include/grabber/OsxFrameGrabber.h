#pragma once

// OSX includes
#ifdef __APPLE__
#include <CoreGraphics/CoreGraphics.h>
#else
#include <grabber/OsxFrameGrabberMock.h>
#endif

// Utils includes
#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

///
/// The OsxFrameGrabber is used for creating snapshots of the display (screenshots)
///
class OsxFrameGrabber : public Grabber
{
public:
	///
	/// Construct a OsxFrameGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] display The index of the display to capture
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	OsxFrameGrabber(unsigned display, unsigned width, unsigned height);
	~OsxFrameGrabber() override;

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	int grabFrame(Image<ColorRgb> & image);

	///
	/// @brief Overwrite Grabber.h implementation
	///
	void setDisplayIndex(int index) override;

private:
	/// display
	unsigned _screenIndex;

	/// Reference to the captured diaplay
	CGDirectDisplayID _display;
};
