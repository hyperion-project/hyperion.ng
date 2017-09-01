#pragma once

// Utils includes
#include <utils/ColorBgr.h>
#include <hyperion/Grabber.h>
#include <grabber/FramebufferFrameGrabber.h>


///
///
class AmlogicGrabber : public Grabber
{
public:
	///
	/// Construct a AmlogicGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] width  The width of the captured screenshot
	/// @param[in] height The heigth of the captured screenshot
	///
	AmlogicGrabber(const unsigned width, const unsigned height);
	~AmlogicGrabber();

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	/// @return Zero on success else negative
	///
	int grabFrame(Image<ColorRgb> & image);

private:
	/**
	 * Returns true if video is playing over the amlogic chip
	 * @return True if video is playing else false
	 */
	bool isVideoPlaying();
	int grabFrame_amvideocap(Image<ColorRgb> & image);
	void closeDev(int &fd);
	bool openDev(int &fd, const char* dev, int flags);

	/** The snapshot/capture device of the amlogic video chip */
	int             _captureDev;
	int             _videoDev;

	Image<ColorBgr> _image_bgr;
	
	int             _lastError;
	bool            _videoPlaying;
	FramebufferFrameGrabber _fbGrabber;
	int             _grabbingModeNotification;
};
