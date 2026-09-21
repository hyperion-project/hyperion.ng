#pragma once

// CoreGraphics
#include <CoreGraphics/CoreGraphics.h>

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
	/// @brief Starts the continuous capture session (macOS 15+).
	///        The ScreenCaptureKit stream is established once and delivers frames asynchronously.
	///        This avoids the expensive shareable content enumeration and the creation of a
	///        short living capture session for every single frame, which pins macOS'
	///        Control Center at high CPU utilisation.
	/// @return True, if the capture session is available (already running or started successfully)
	///
	bool startStream();

	///
	/// @brief Stops the continuous capture session and releases the capture session state
	///
	void stopStream();

	///
	/// @brief Determine whether the continuous capture session is running
	///
	bool isStreamActive();

	///
	/// Captures a single snapshot of the display and writes the data to the given image. The
	/// provided image should have the same dimensions as the configured values (_width and
	/// _height)
	///
	/// @param[out] image  The snapped screenshot (should be initialized with correct width and
	/// height)
	///
	int grabFrame(Image<ColorRgb> & image, bool forceUpdate = false) override;

	///
	/// @brief Overwrite Grabber.h implementation
	///
	bool setDisplayIndex(int index) override;

	///
	/// @brief Overwrite Grabber.h implementation.
	///        The capture frequency is applied to the continuous capture session, too.
	///
	bool setFramerate(int fps) override;

	///
	/// @brief Overwrite Grabber.h implementation.
	///        The pixel decimation defines the resolution the display is scaled to by the
	///        capture session, i.e. the capture session is re-created on changes.
	///
	bool setPixelDecimation(int decimation) override;

	///
	/// @brief Overwrite Grabber.h implementation.
	///        Cropping is applied by the ImageResampler, which requires the display to be
	///        captured in its native resolution.
	///
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom) override;

	///
	/// @brief Overwrite Grabber.h implementation.
	///        The 3D modes are handled by the ImageResampler, which requires the display to be
	///        captured in its native resolution.
	///
	void setVideoMode(VideoMode mode) override;

	///
	/// @brief Discover OSX screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params);

private:
	///
	/// @brief Creates the capture session for the current display (ScreenCaptureKit content filter)
	/// @return True on success
	///
	bool setupCaptureSession();

	///
	/// @brief Releases the continuous capture session (stream and frame receiver)
	///
	void teardownStream();

	///
	/// @brief Takes a single screenshot of the current capture session and writes it to the image
	/// @return 0 on success, -1 otherwise
	///
	int grabSnapshot(Image<ColorRgb> & image);

	///
	/// @brief Converts a CGImage into the given image
	/// @return 0 on success, -1 otherwise
	///
	int convertCGImage(CGImageRef imageRef, Image<ColorRgb> & image);

	///
	/// @brief Determine whether the display is scaled down by the capture session (GPU) instead of
	///        converting the full resolution image via the ImageResampler.
	///        Only possible if neither cropping nor a 3D mode is used.
	///
	bool useGpuScaling() const;

	///
	/// @brief Resolution (in pixels) the display is captured with, i.e. points scaled by the
	///        display's backing scale factor
	///
	QSize captureSize() const;

	///
	/// @brief Resolution of the image handed over to Hyperion, i.e. the capture resolution
	///        divided by the pixel decimation (same calculation as ImageResampler::processImage)
	///
	QSize analysisSize() const;

	///
	/// @brief Applies the capture mode (GPU scaling vs. ImageResampler) to the ImageResampler and
	///        re-creates the capture session if the effective capture configuration changed
	///
	void applyCaptureMode();

	/// display
	int _screenIndex;

	/// Reference to the captured display
	CGDirectDisplayID _display;

	/// Continuous capture session (SCStream), nullptr if not available.
	/// Kept as void* to not expose ScreenCaptureKit types via this header
	void* _stream;

	/// Receiver of the frames delivered by the continuous capture session (OsxStreamOutput)
	void* _streamOutput;

	/// Content filter of the capture session (SCContentFilter) - reused for the snapshot fallback
	void* _contentFilter;

	/// Timestamp (in milliseconds) of the last attempt to establish the continuous capture session
	int64_t _lastStreamAttemptMs;

	/// Timestamp (in milliseconds) when the continuous capture session was started, 0 when not running
	int64_t _streamStartedMs;

	/// True, if the display is scaled down by the capture session (GPU)
	bool _gpuScalingMode;

	/// Resolution the active capture session is configured for
	QSize _streamSize;
};
