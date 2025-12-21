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
	explicit FramebufferFrameGrabber(int deviceIdx = 0);

	~FramebufferFrameGrabber() override;

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	int grabFrame(Image<ColorRgb> & image) override;

	///
	/// @brief Setup a new capture screen, will free the previous one
	/// @return True on success, false if no screen is found
	///
	bool setupScreen() override;


	QSize getScreenSize() const override;
	QSize getScreenSize(const QString& device) const;

	///
	///@brief Set new width and height for framegrabber, overwrite Grabber.h implementation
	bool setWidthHeight(int width, int height) override;

	QString getDeviceName() const {return QString("/dev/fb%1").arg(_input);}

	QJsonArray getInputDeviceDetails() const override;

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

	int _deviceFd;
	struct fb_var_screeninfo _varInfo;
	struct fb_fix_screeninfo _fixInfo;

	PixelFormat _pixelFormat;
};
