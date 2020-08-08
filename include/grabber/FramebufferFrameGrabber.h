#pragma once

// Utils includes
#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

///
/// The FramebufferFrameGrabber is used for creating snapshots of the display (screenshots)
///
class FramebufferFrameGrabber : public Grabber
{
public:
	///
	/// Construct a FramebufferFrameGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] device The framebuffer device name/path
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	FramebufferFrameGrabber(const QString & device, unsigned width, unsigned height);

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
	/// @brief Overwrite Grabber.h implememtation
	///
	void setDevicePath(const QString& path) override;

private:
	/// Framebuffer device e.g. /dev/fb0
	QString _fbDevice;
};
