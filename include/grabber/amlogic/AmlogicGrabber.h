#pragma once

// Utils includes
#include <utils/ColorBgr.h>
#include <utils/ColorRgba.h>
#include <hyperion/Grabber.h>
#include <grabber/framebuffer/FramebufferFrameGrabber.h>
#include <grabber/drm/DRMFrameGrabber.h>

///
///
class AmlogicGrabber : public Grabber
{
public:
	///
	/// Construct a AmlogicGrabber that will capture snapshots with specified dimensions.
	///
	///
	explicit AmlogicGrabber(int deviceIdx = 0);
	~AmlogicGrabber() override;

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	/// @return Zero on success else negative
	///
	int grabFrame(Image<ColorRgb> &image) override;

	///
	/// @brief Discover AmLogic screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject &params);

	///
	/// @brief Apply new width/height values, on errors (collide with cropping) reject the values
	/// @return True on success else false
	///
	bool setWidthHeight(int width, int height) override;

private:
	bool isGbmSupported() const;

	/**
	 * Returns true if video is playing over the amlogic chip
	 * @return True if video is playing else false
	 */
	bool isVideoPlaying();
	void closeDevice(int &fd) const;
	bool openDevice(int &fd, const char *dev) const;

	int grabFrame_amvideocap(Image<ColorRgb> &image);

	/** The snapshot/capture device of the amlogic video chip */
	int _captureDev;
	int _videoDev;

	Image<ColorBgr> _image_bgr;
	ColorBgr* _image_ptr;

	int _lastError;
	int _grabbingModeNotification;
};
