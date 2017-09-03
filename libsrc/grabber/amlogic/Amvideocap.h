#pragma once

#include <exception>
// #include <linux/videodev2.h>
#include "ion.h"
#include "meson_ion.h"
#include "IonBuffer.h"

#define AMVIDEOCAP_IOC_MAGIC  'V'
#define CAP_FLAG_AT_CURRENT		0
#define CAP_FLAG_AT_TIME_WINDOW	1
#define CAP_FLAG_AT_END			2

/*
format see linux/ge2d/ge2d.h
like:
GE2D_FORMAT_S24_RGB
*/
#define GE2D_ENDIAN_SHIFT	24
#define GE2D_LITTLE_ENDIAN          (1 << GE2D_ENDIAN_SHIFT)
#define GE2D_COLOR_MAP_SHIFT        20
#define GE2D_COLOR_MAP_BGR888	(5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB888       (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_FMT_S24_RGB (GE2D_LITTLE_ENDIAN|0x00200) /* 10_00_0_00_0_00 */
#define GE2D_FORMAT_S24_BGR (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_BGR888) 
#define GE2D_FORMAT_S24_RGB (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGB888)

// #define AMVIDEOCAP_IOW_SET_WANTFRAME_FORMAT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x01, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH      		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)

#define _A_M  'S'
#define AMSTREAM_IOC_GET_VIDEO_DISABLE  _IOR((_A_M), 0x48, int)

#define AMVIDEO_MAGIC  'X'

#define AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME _IOR((AMVIDEO_MAGIC), 0x01, int)
#define AMVIDEO_EXT_PUT_CURRENT_VIDEOFRAME _IO((AMVIDEO_MAGIC), 0x02)

#define AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT _IOR((AMVIDEO_MAGIC), 0x03, uint32_t)
#define AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_SIZE _IOR((AMVIDEO_MAGIC), 0x04, uint64_t)
#define AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR _IOR((AMVIDEO_MAGIC), 0x05, uint32_t)

// GE2D commands
#define	GE2D_STRETCHBLIT_NOALPHA            0x4702
#define	GE2D_CONFIG_EX                      0x46fa


// data structures
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


enum ge2d_src_dst_e {
	OSD0_OSD0 = 0,
	OSD0_OSD1,
	OSD1_OSD1,
	OSD1_OSD0,
	ALLOC_OSD0,
	ALLOC_OSD1,
	ALLOC_ALLOC,
	TYPE_INVALID,
};

enum ge2d_src_canvas_type_e {
	CANVAS_OSD0 = 0,
	CANVAS_OSD1,
	CANVAS_ALLOC,
	CANVAS_TYPE_INVALID,
};


struct src_dst_para_s {
	int  xres;
	int  yres;
	int  canvas_index;
	int  bpp;
	int  ge2d_color_index;
};

enum ge2d_op_type_e {
	GE2D_OP_DEFAULT = 0,
	GE2D_OP_FILLRECT,
	GE2D_OP_BLIT,
	GE2D_OP_STRETCHBLIT,
	GE2D_OP_BLEND,
	GE2D_OP_MAXNUM
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
