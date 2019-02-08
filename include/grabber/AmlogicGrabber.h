#pragma once

// Utils includes
#include <utils/ColorBgr.h>
#include <utils/ColorRgba.h>
#include <hyperion/Grabber.h>
#include <grabber/FramebufferFrameGrabber.h>

class IonBuffer;

struct rectangle_s {
	int x;   /* X coordinate of its top-left point */
	int y;   /* Y coordinate of its top-left point */
	int w;   /* width of it */
	int h;   /* height of it */
};

struct ge2d_para_s {
	unsigned int    color;
	struct rectangle_s src1_rect;
	struct rectangle_s src2_rect;
	struct rectangle_s dst_rect;
	int op;
};

struct config_planes_s {
	unsigned long addr;
	unsigned int w;
	unsigned int h;
};

struct src_key_ctrl_s {
	int key_enable;
	int key_color;
	int key_mask;
	int key_mode;
};

struct config_para_s {
	int  src_dst_type;
	int  alu_const_color;
	unsigned int src_format;
	unsigned int dst_format; /* add for src&dst all in user space. */

	struct config_planes_s src_planes[4];
	struct config_planes_s dst_planes[4];
	struct src_key_ctrl_s  src_key;
};

struct src_dst_para_ex_s {
	int  canvas_index;
	int  top;
	int  left;
	int  width;
	int  height;
	int  format;
	int  mem_type;
	int  color;
	unsigned char x_rev;
	unsigned char y_rev;
	unsigned char fill_color_en;
	unsigned char fill_mode;
};

struct config_para_ex_s {
	struct src_dst_para_ex_s src_para;
	struct src_dst_para_ex_s src2_para;
	struct src_dst_para_ex_s dst_para;

	/* key mask */
	struct src_key_ctrl_s  src_key;
	struct src_key_ctrl_s  src2_key;

	int alu_const_color;
	unsigned src1_gb_alpha;
	unsigned op_mode;
	unsigned char bitmask_en;
	unsigned char bytemask_only;
	unsigned int  bitmask;
	unsigned char dst_xy_swap;

	/* scaler and phase releated */
	unsigned hf_init_phase;
	int hf_rpt_num;
	unsigned hsc_start_phase_step;
	int hsc_phase_slope;
	unsigned vf_init_phase;
	int vf_rpt_num;
	unsigned vsc_start_phase_step;
	int vsc_phase_slope;
	unsigned char src1_vsc_phase0_always_en;
	unsigned char src1_hsc_phase0_always_en;
	/* 1bit, 0: using minus, 1: using repeat data */
	unsigned char src1_hsc_rpt_ctrl;
	/* 1bit, 0: using minus  1: using repeat data */
	unsigned char src1_vsc_rpt_ctrl;

	/* canvas info */
	struct config_planes_s src_planes[4];
	struct config_planes_s src2_planes[4];
	struct config_planes_s dst_planes[4];
};

struct amvideo_grabber_data {
	int canvas_index;
	uint32_t canvas0Addr;
	uint32_t ge2dformat;
	uint64_t size;
};

enum ge2d_mode {
    ge2d_single = 0,
    ge2d_combined = 1
};

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
	/// @param[in] ge2d_mode The ge2d mode, 0: single icotl calls, 1: combined data ioctl call
	///
	AmlogicGrabber(const unsigned width, const unsigned height, const unsigned ge2d_mode, const QString device);
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
	void closeDev(int &fd);
	bool openDev(int &fd, const char* dev);

	int grabFrame_amvideocap(Image<ColorRgb> & image);
	int grabFrame_ge2d(Image<ColorRgb> & image);

	/** The snapshot/capture device of the amlogic video chip */
	int             _captureDev;
	int             _videoDev;
	int             _ge2dDev;

	Image<ColorBgr> _image_bgr;
	void*           _image_ptr;
	ssize_t         _bytesToRead;

	int             _lastError;
	bool            _videoPlaying;
	FramebufferFrameGrabber _fbGrabber;
	int             _grabbingModeNotification;
	void*           _ge2dVideoBufferPtr;
	IonBuffer*      _ge2dIonBuffer;
	struct config_para_ex_s _configex;
	ge2d_para_s     _blitRect;
	int             _ge2d_mode;
	QString         _device;
};
