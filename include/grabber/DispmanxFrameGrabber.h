#pragma once

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgba.h>
#include <hyperion/Grabber.h>

// dynamic linking
#include <dlfcn.h>

// BCM includes
#ifdef BCM_FOUND
	#include <bcm_host.h>
#else
	typedef int DISPMANX_DISPLAY_HANDLE_T;
	typedef Image<ColorRgba> DISPMANX_RESOURCE;
	typedef DISPMANX_RESOURCE* DISPMANX_RESOURCE_HANDLE_T;
	typedef int VC_IMAGE_TYPE_T, DISPMANX_TRANSFORM_T;
	const int VC_IMAGE_RGBA32 = 1, DISPMANX_SNAPSHOT_FILL = 1;
	struct VC_RECT_T { int left, top, width, height; };
	struct DISPMANX_MODEINFO_T { int width, height; uint32_t display_num; };
#endif

///
/// The DispmanxFrameGrabber is used for creating snapshots of the display (screenshots) with a downsized and scaled resolution.
///
class DispmanxFrameGrabber : public Grabber
{
public:
	///
	/// Construct a DispmanxFrameGrabber that will capture snapshots with specified dimensions.
	///
	DispmanxFrameGrabber();
	~DispmanxFrameGrabber() override;

	///
	/// @brief Determine if the bcm library is available.
	///
	/// @return Zero, on success (i.e. library is present), else negative
	///
	bool isAvailable();

	///
	/// @brief Opens the input device.
	///
	/// @return Zero, on success (i.e. device is ready), else negative
	///
	bool open();

	///
	/// @brief Setup a new capture screen, will free the previous one
	/// @return True on success, false if no screen is found
	///
	bool setupScreen();

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
	///@brief Set new width and height for dispmanx, overwrite Grabber.h impl
	bool setWidthHeight(int width, int height) override;

	QSize getScreenSize(int display=0) const;

	///
	/// @brief Discover DispmanX screens available (for configuration).
	///
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	///
	/// @return A JSON structure holding a list of devices found
	///
	QJsonObject discover(const QJsonObject& params);

private:
	///
	/// Updates the frame-grab flags as used by the VC library for frame grabbing
	///
	/// @param vc_flags  The snapshot grabbing mask
	///
	void setFlags(DISPMANX_TRANSFORM_T vc_flags);

	///
	/// @brief free _vc_resource and captureBuffer
	///
	void freeResources();

	void* _lib;

	/// Handle to the display that is being captured
	DISPMANX_DISPLAY_HANDLE_T _vc_display;

	/// Handle to the resource for storing the captured snapshot
	DISPMANX_RESOURCE_HANDLE_T _vc_resource;

	/// Rectangle of the captured resource that is transferred to user space
	VC_RECT_T _rectangle;

	/// Flags (transforms) for creating snapshots
	DISPMANX_TRANSFORM_T _vc_flags;

	// temp buffer when capturing with unsupported pitch size or
	// when we need to crop the image
	ColorRgba* _captureBuffer;

	// size of the capture buffer in Pixels
	unsigned _captureBufferSize;

	// rgba output buffer
	Image<ColorRgba>  _image_rgba;

private:
	void (*wr_bcm_host_init)(void);
	void (*wr_bcm_host_deinit)(void);
	DISPMANX_DISPLAY_HANDLE_T (*wr_vc_dispmanx_display_open)(uint32_t device);
	int (*wr_vc_dispmanx_display_close)(DISPMANX_DISPLAY_HANDLE_T display);
	int (*wr_vc_dispmanx_display_get_info)(DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_MODEINFO_T *pinfo);
	DISPMANX_RESOURCE_HANDLE_T (*wr_vc_dispmanx_resource_create)(VC_IMAGE_TYPE_T type, uint32_t width, uint32_t height, uint32_t *native_image_handle);
	int (*wr_vc_dispmanx_resource_delete)(DISPMANX_RESOURCE_HANDLE_T res);
	int (*wr_vc_dispmanx_resource_read_data)(DISPMANX_RESOURCE_HANDLE_T handle, const VC_RECT_T *p_rect, void *dst_address, uint32_t dst_pitch);
	void (*wr_vc_dispmanx_rect_set)(VC_RECT_T *rectangle, int left, int top, int width, int height);
	int (*wr_vc_dispmanx_snapshot) (DISPMANX_DISPLAY_HANDLE_T display, DISPMANX_RESOURCE_HANDLE_T snapshot_resource, DISPMANX_TRANSFORM_T transform);
};
