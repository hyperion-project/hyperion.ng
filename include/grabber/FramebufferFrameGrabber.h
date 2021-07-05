#pragma once

#include <linux/fb.h>

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
	///
	FramebufferFrameGrabber(const QString & device="/dev/fb0");

	~FramebufferFrameGrabber() override;

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
	/// @brief Setup a new capture screen, will free the previous one
	/// @return True on success, false if no screen is found
	///
	bool setupScreen();


	QSize getScreenSize() const;
	QSize getScreenSize(const QString& device) const;

	///
	///@brief Set new width and height for framegrabber, overwrite Grabber.h implementation
	bool setWidthHeight(int width, int height) override;

	QString getPath() const {return _fbDevice;}

	///
	/// @brief Discover Framebuffer screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params);

private:

	bool openDevice();
	bool closeDevice();
	bool getScreenInfo();

	/// Framebuffer device e.g. /dev/fb0
	QString _fbDevice;

	int _fbfd;
	struct fb_var_screeninfo _varInfo;
	struct fb_fix_screeninfo _fixInfo;

	PixelFormat _pixelFormat;
};
