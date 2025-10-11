#pragma once

// DRM
#include <drm_fourcc.h>
#include <xf86drmMode.h>
#include <xf86drm.h>

// Utils includes
#include <utils/ColorRgb.h>
#include <hyperion/Grabber.h>

struct DrmProperty
{
	drmModePropertyPtr spec;
	uint64_t value;
};

struct Connector
{
	drmModeConnectorPtr ptr;
	std::map<std::string, DrmProperty> props;
};

struct Encoder
{
	drmModeEncoderPtr ptr;
	std::map<std::string, DrmProperty> props;
};

///
/// The DRMFrameGrabber is used for creating snapshots of the display (screenshots)
///
class DRMFrameGrabber : public Grabber
{
public:
	///
	/// Construct a DRMFrameGrabber that will capture snapshots with specified dimensions.
	///
	/// @param[in] device The framebuffer device name/path
	///
	DRMFrameGrabber(int deviceIdx = 0, int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);

	~DRMFrameGrabber() override;

	void freeResources();

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
	///@brief Set new width and height for the DRM-grabber, overwrite Grabber.h implementation
	bool setWidthHeight(int width, int height) override;

	QString getDeviceName() const {return QString("%1/%2%3").arg(DRM_DIR_NAME, DRM_PRIMARY_MINOR_NAME).arg(_input);}

	///
	/// @brief Discover DRM screens available (for configuration).
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
    std::map<uint32_t, Connector*> connectors;
    std::map<uint32_t, Encoder*> encoders;
    drmModeCrtcPtr crtc;
    std::map<uint32_t, drmModePlanePtr> planes;
	std::map<uint32_t, drmModeFB2Ptr> framebuffers;

	PixelFormat _pixelFormat;
};

