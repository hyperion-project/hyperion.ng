#pragma once

#include <QScopedPointer>

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
	/// @brief Setup a new capture screen, will free the previous one
	/// @return True on success, false if no screen is found
	///
	bool setupScreen() override;

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
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(VideoMode mode) override;

	///
	/// @brief Apply new crop values, on errors reject the values
	///
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom) override;

	///
	/// @brief Apply new width/height values, on errors (collide with cropping) reject the values
	/// @return True on success else false
	///
	bool setWidthHeight(int width, int height) override;

	///
	/// @brief Apply new framerate
	/// @param fps framesPerSecond
	///
	bool setFramerate(int fps) override;

	///
	/// @brief  Apply new pixelDecimation
	///
	bool setPixelDecimation(int pixelDecimation) override;

private:

	bool isGbmSupported(bool logMsg = true) const;

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
	bool _videoPlaying;
	QScopedPointer<Grabber>	_screenGrabber;
	int _grabbingModeNotification;
};
