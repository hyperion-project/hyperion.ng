#pragma once

#include <map>
#include <memory>
#include <vector>

// DRM
#include <drm_fourcc.h>
#include <xf86drmMode.h>
#include <xf86drm.h>

// Utils includes
#include <utils/ColorRgb.h>
#include <utils/Logger.h>
#include <hyperion/Grabber.h>
#include <QLoggingCategory>

// Utility includes
#include <utils/Logger.h>

struct DrmProperty
{
	drmModePropertyPtr spec;
	uint64_t value;
};

struct Connector
{
	drmModeConnectorPtr ptr;
	std::map<std::string, DrmProperty, std::less<>> props;
};

struct Encoder
{
	drmModeEncoderPtr ptr;
	std::map<std::string, DrmProperty, std::less<>> props;
};

struct DrmResources {
	using drmModeConnectorPtr_unique = std::unique_ptr<drmModeConnector, decltype(&drmModeFreeConnector)>;
	using drmModeCrtcPtr_unique = std::unique_ptr<drmModeCrtc, decltype(&drmModeFreeCrtc)>;

	std::vector<drmModeConnectorPtr_unique> connectors;
	std::vector<drmModeCrtcPtr_unique> crtcs;
};

///
/// The DRMFrameGrabber is used for creating snapshots of the display (screenshots)
///
class DRMFrameGrabber : public Grabber
/**
 * @brief The DRMFrameGrabber class is a screen grabber that uses the Linux Direct Rendering Manager (DRM)
 * API to capture the screen content. This method is generally faster and more efficient than X11-based
 * methods, as it operates at a lower level. It is suitable for systems without a running X server,
 * such as dedicated media centers.
 */
{
public:
	/**
	 * @brief Constructs a DRMFrameGrabber.
	 *
	 * @param deviceIdx The index of the DRM device to use (e.g., 0 for /dev/dri/card0).
	 * @param cropLeft Number of pixels to crop from the left of the screen.
	 * @param cropRight Number of pixels to crop from the right of the screen.
	 * @param cropTop Number of pixels to crop from the top of the screen.
	 * @param cropBottom Number of pixels to crop from the bottom of the screen.
	 */
	explicit DRMFrameGrabber(int deviceIdx = 0, int cropLeft=0, int cropRight=0, int cropTop=0, int cropBottom=0);

	/**
	 * @brief Destructor for the DRMFrameGrabber.
	 * Ensures that all DRM resources are properly released by calling freeResources().
	 */
	~DRMFrameGrabber() override;

	/**
	 * @brief Captures a single frame from the configured DRM device.
	 * The captured frame is processed (cropped and scaled) and stored in the provided image object.
	 *
	 * @param[out] image The Image object to store the captured frame. It must be initialized
	 *                   with the correct dimensions before calling this function.
	 * @return 0 on success, a negative value on failure.
	 */
	int grabFrame(Image<ColorRgb> & image) override;

	/**
	 * @brief Initializes the DRM device for screen capturing.
	 * This involves opening the device, identifying active connectors, CRTCs, and planes,
	 * and setting up the necessary resources for grabbing frames.
	 *
	 * @return True if the screen was set up successfully, false otherwise.
	 */
	bool setupScreen() override;

	/**
	 * @brief Gets the resolution of the currently configured screen.
	 *
	 * @return A QSize object containing the width and height of the screen.
	 */
	QSize getScreenSize() const override;

	/**
	 * @brief Gets the resolution of a specific DRM device.
	 *
	 * @param device The name of the device (e.g., "card0").
	 * @return A QSize object containing the width and height of the specified screen.
	 */
	QSize getScreenSize(const QString& device) const;

	/**
	 * @brief Sets the desired capture width and height.
	 * This will affect the dimensions of the image returned by grabFrame().
	 *
	 * @param width The desired capture width.
	 * @param height The desired capture height.
	 * @return True on success, false on failure.
	 */
	bool setWidthHeight(int width, int height) override;

	/**
	 * @brief Returns the full device path for the current DRM grabber instance.
	 * For example, "/dev/dri/card0".
	 *
	 * @return A QString containing the device name.
	 */
	QString getDeviceName() const {return QString("%1/%2%3").arg(DRM_DIR_NAME, DRM_PRIMARY_MINOR_NAME).arg(_input);}

	/**
	 * @brief Retrieves a list of available DRM input devices.
	 *
	 * @return A QJsonArray where each element is a QJsonObject describing a found device.
	 */
	QJsonArray getInputDeviceDetails() const override;

	/**
	 * @brief Discovers available DRM devices and their properties.
	 * This is used for configuration purposes, allowing a user to see available screens.
	 *
	 * @param params JSON object with parameters to customize the discovery process.
	 * @return A QJsonObject containing a list of discovered devices and their details.
	 */
	QJsonObject discover(const QJsonObject& params);

private:

	/**
	 * @brief Releases all allocated DRM resources.
	 * This includes closing the device file descriptor and freeing memory associated with
	 * connectors, encoders, CRTCs, planes, and framebuffers.
	 */
	void freeResources();

	/**
	 * @brief Opens the DRM device file descriptor.
	 * @return True on success, false on failure.
	 */
	bool openDevice();

	/**
	 * @brief Closes the DRM device file descriptor.
	 * @return True on success, false on failure.
	 */
	bool closeDevice();

	/**
	 * @brief Gathers information about the active screen configuration.
	 * This includes enumerating connectors, encoders, finding the active CRTC and primary plane.
	 * @return True on success, false on failure.
	 */
	bool getScreenInfo();

	/**
	 * @brief Helper function to discover DRM resources for a given device.
	 * @param[in] device The device path (e.g., "/dev/dri/card0").
	 * @param[out] resources A struct to be filled with the discovered resource information.
	 * @return True on success, false on failure.
	 */
	bool discoverDrmResources(const QString& device, DrmResources& resources) const;

	/**
	 * @brief Sets the client capabilities for the DRM device connection.
	 * Specifically, it requests universal plane support.
	 */
	void setDrmClientCaps();

	/**
	 * @brief Enumerates all available connectors and encoders for the device.
	 * @param resources The DRM resources structure obtained from the device.
	 */
	void enumerateConnectorsAndEncoders(const drmModeRes* resources);

	/**
	 * @brief Finds the active CRTC (CRT Controller) that is connected to a display.
	 * @param resources The DRM resources structure obtained from the device.
	 */
	void findActiveCrtc(const drmModeRes* resources);

	/**
	 * @brief Checks if a given plane is the primary plane for the active CRTC.
	 * @param planeId The ID of the plane to check.
	 * @param properties The properties of the plane object.
	 * @return True if it is the primary plane, false otherwise.
	 */
	bool isPrimaryPlaneForCrtc(uint32_t planeId, const drmModeObjectProperties* properties);

	/**
	 * @brief Finds the primary display plane associated with the active CRTC.
	 * The primary plane is the one that holds the main desktop image.
	 * @param planeResources The list of available planes.
	 */
	void findPrimaryPlane(const drmModePlaneRes* planeResources);

	/**
	 * @brief Retrieves properties for various DRM objects like connectors and planes.
	 * This is currently a placeholder and not fully implemented.
	 */
	void getDrmObjectProperties() const;

	/**
	 * @brief Retrieves the framebuffer(s) associated with the primary plane.
	 * This is necessary to get a handle to the screen's pixel data.
	 */
	void getFramebuffers();

	/// The file descriptor for the opened DRM device.
	int _deviceFd;

	/// Map of available connectors (e.g., HDMI, DP), keyed by connector ID.
	std::map<uint32_t, std::unique_ptr<Connector>> _connectors;

	/// Map of available encoders, keyed by encoder ID.
	std::map<uint32_t, std::unique_ptr<Encoder>> _encoders;

	/// Pointer to the active CRTC (CRT Controller).
	drmModeCrtcPtr _crtc;

	/// Map of available display planes, keyed by plane ID.
	std::map<uint32_t, drmModePlanePtr> _planes;

	/// Map of framebuffers attached to the CRTC, keyed by framebuffer ID.
	std::map<uint32_t, drmModeFB2Ptr> _framebuffers;

	/// The pixel format of the captured framebuffer.
	PixelFormat _pixelFormat;
};

