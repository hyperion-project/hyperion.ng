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

	///
	OsxFrameGrabber(int display=kCGDirectMainDisplay);
	~OsxFrameGrabber() override;

	///
	/// @brief Setup a new capture screen, will free the previous one
	/// @return True on success, false if no screen is found
	///
	bool setupDisplay();

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
	bool setDisplayIndex(int index) override;

	///
	/// @brief Discover OSX screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params);

private:
	/// display
	int _screenIndex;

	/// Reference to the captured display
	CGDirectDisplayID _display;
};
