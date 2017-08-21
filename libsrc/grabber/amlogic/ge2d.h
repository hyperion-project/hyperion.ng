/*
 * include/linux/amlogic/ge2d/ge2d.h
 *
 * Copyright (C) 2015 Amlogic, Inc. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 * more details.
 *
*/


#ifndef _GE2D_H_
#define _GE2D_H_


#define MAX_BITBLT_WORK_CONFIG 4
#define MAX_GE2D_CMD  32   /* 64 */

#define GE2D_STATE_IDLE                 0
#define GE2D_STATE_RUNNING              1
#define GE2D_STATE_CLEANUP              2
#define GE2D_STATE_REMOVING_WQ          3
#define	GE2D_PROCESS_QUEUE_START        0
#define	GE2D_PROCESS_QUEUE_STOP         1

#define RELEASE_SRC1_CANVAS 0x01
#define RELEASE_SRC2_CANVAS 0x02
#define RELEASE_SRC1_BUFFER 0x04
#define RELEASE_SRC2_BUFFER 0x08
#define RELEASE_CB          0x10
#define RELEASE_REQUIRED    0x1f

#define START_FLAG          0x20
#define RELEASE_FLAG        0x40
#define FINISH_FLAG         0x80

#define FORMAT_8BIT_COMPONENT   0
#define COMPONENT_Y_OR_R      0
#define COMPONENT_Cb_OR_G     1
#define COMPONENT_Cr_OR_B     2
#define COMPONENT_ALPHA       3
#define FORMAT_422_YUV          1
#define FORMAT_444_YUV_OR_RGB   2
#define FORMAT_YUVA_OR_RGBA     3

#define FILL_MODE_BOUNDARY_PIXEL    0
#define FILL_MODE_DEFAULT_COLOR     1

#define OPERATION_ADD           0    /* Cd = Cs*Fs+Cd*Fd */
#define OPERATION_SUB           1    /* Cd = Cs*Fs-Cd*Fd */
#define OPERATION_REVERSE_SUB   2    /* Cd = Cd*Fd-Cs*Fs */
#define OPERATION_MIN           3    /* Cd = Min(Cd*Fd,Cs*Fs) */
#define OPERATION_MAX           4    /* Cd = Max(Cd*Fd,Cs*Fs) */
#define OPERATION_LOGIC         5

#define COLOR_FACTOR_ZERO                     0
#define COLOR_FACTOR_ONE                      1
#define COLOR_FACTOR_SRC_COLOR                2
#define COLOR_FACTOR_ONE_MINUS_SRC_COLOR      3
#define COLOR_FACTOR_DST_COLOR                4
#define COLOR_FACTOR_ONE_MINUS_DST_COLOR      5
#define COLOR_FACTOR_SRC_ALPHA                6
#define COLOR_FACTOR_ONE_MINUS_SRC_ALPHA      7
#define COLOR_FACTOR_DST_ALPHA                8
#define COLOR_FACTOR_ONE_MINUS_DST_ALPHA      9
#define COLOR_FACTOR_CONST_COLOR              10
#define COLOR_FACTOR_ONE_MINUS_CONST_COLOR    11
#define COLOR_FACTOR_CONST_ALPHA              12
#define COLOR_FACTOR_ONE_MINUS_CONST_ALPHA    13
#define COLOR_FACTOR_SRC_ALPHA_SATURATE       14

#define ALPHA_FACTOR_ZERO                     0
#define ALPHA_FACTOR_ONE                      1
#define ALPHA_FACTOR_SRC_ALPHA                2
#define ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA      3
#define ALPHA_FACTOR_DST_ALPHA                4
#define ALPHA_FACTOR_ONE_MINUS_DST_ALPHA      5
#define ALPHA_FACTOR_CONST_ALPHA              6
#define ALPHA_FACTOR_ONE_MINUS_CONST_ALPHA    7

#define LOGIC_OPERATION_CLEAR       0
#define LOGIC_OPERATION_COPY        1
#define LOGIC_OPERATION_NOOP        2
#define LOGIC_OPERATION_SET         3
#define LOGIC_OPERATION_COPY_INVERT 4
#define LOGIC_OPERATION_INVERT      5
#define LOGIC_OPERATION_AND_REVERSE 6
#define LOGIC_OPERATION_OR_REVERSE  7
#define LOGIC_OPERATION_AND         8
#define LOGIC_OPERATION_OR          9
#define LOGIC_OPERATION_NAND        10
#define LOGIC_OPERATION_NOR         11
#define LOGIC_OPERATION_XOR         12
#define LOGIC_OPERATION_EQUIV       13
#define LOGIC_OPERATION_AND_INVERT  14
#define LOGIC_OPERATION_OR_INVERT   15

#define DST_CLIP_MODE_INSIDE    0
#define DST_CLIP_MODE_OUTSIDE   1

#define FILTER_TYPE_BICUBIC     1
#define FILTER_TYPE_BILINEAR    2
#define FILTER_TYPE_TRIANGLE    3

#define MATRIX_YCC_TO_RGB               1
#define MATRIX_RGB_TO_YCC               2
#define MATRIX_FULL_RANGE_YCC_TO_RGB    3


#define GE2D_ENDIAN_SHIFT	24
#define GE2D_ENDIAN_MASK            (0x1 << GE2D_ENDIAN_SHIFT)
#define GE2D_BIG_ENDIAN             (0 << GE2D_ENDIAN_SHIFT)
#define GE2D_LITTLE_ENDIAN          (1 << GE2D_ENDIAN_SHIFT)

#define GE2D_COLOR_MAP_SHIFT        20
#define GE2D_COLOR_MAP_MASK         (0xf << GE2D_COLOR_MAP_SHIFT)
/* nv12 &nv21, only works on m6*/
#define GE2D_COLOR_MAP_NV12		(15 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_NV21		(14 << GE2D_COLOR_MAP_SHIFT)
/* 16 bit */
#define GE2D_COLOR_MAP_YUV422		(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB655		(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV655		(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB844		(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV844		(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6442     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6442     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4444     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4444     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB565       (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV565       (5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB4444		(6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV4444		(6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB1555     (7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV1555     (7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4642     (8 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4642     (8 << GE2D_COLOR_MAP_SHIFT)
/* 24 bit */
#define GE2D_COLOR_MAP_RGB888       (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV444       (0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA5658     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA5658     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8565     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8565     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6666     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6666     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB6666     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV6666     (4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGR888		(5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUY888		(5 << GE2D_COLOR_MAP_SHIFT)
/* 32 bit */
#define GE2D_COLOR_MAP_RGBA8888		(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA8888		(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8888     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8888     (1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ABGR8888     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AVUY8888     (2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGRA8888     (3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUYA8888     (3 << GE2D_COLOR_MAP_SHIFT)

/* format code is defined as:
[11] : 1-YUV color space, 0-RGB color space
[10] : compress_range
[9:8]: format
[7:6]: 8bit_mode_sel
[5]  : LUT_EN
[4:3]: PIC_STRUCT
[2]  : SEP_EN
[1:0]: X_YC_RATIO, SRC1_Y_YC_RATIO
*/
#define GE2D_FORMAT_MASK                0x0ffff
#define GE2D_BPP_MASK                   0x00300
#define GE2D_BPP_8BIT                   0x00000
#define GE2D_BPP_16BIT                  0x00100
#define GE2D_BPP_24BIT                  0x00200
#define GE2D_BPP_32BIT                  0x00300
#define GE2D_FORMAT_YUV                 0x20000
#define GE2D_FORMAT_COMP_RANGE          0x10000
/*bit8(2)  format   bi6(2) mode_8b_sel  bit5(1)lut_en   bit2 sep_en*/
/*M  seperate block S one block.*/

#define GE2D_FMT_S8_Y		0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_CB		0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_CR		0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_R		0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_G		0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_B		0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_A		0x000c0 /* 00_11_0_00_0_00 */
#define GE2D_FMT_S8_LUT		0x00020 /* 00_00_1_00_0_00 */
#define GE2D_FMT_S16_YUV422	0x20102 /* 01_00_0_00_0_00 */
#define GE2D_FMT_S16_RGB (GE2D_LITTLE_ENDIAN|0x00100) /* 01_00_0_00_0_00 */
#define GE2D_FMT_S24_YUV444	0x20200 /* 10_00_0_00_0_00 */
#define GE2D_FMT_S24_RGB (GE2D_LITTLE_ENDIAN|0x00200) /* 10_00_0_00_0_00 */
#define GE2D_FMT_S32_YUVA444	0x20300 /* 11_00_0_00_0_00 */
#define GE2D_FMT_S32_RGBA (GE2D_LITTLE_ENDIAN|0x00300) /* 11_00_0_00_0_00 */
#define GE2D_FMT_M24_YUV420	0x20007 /* 00_00_0_00_1_11 */
#define GE2D_FMT_M24_YUV422	0x20006 /* 00_00_0_00_1_10 */
#define GE2D_FMT_M24_YUV444	0x20004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_RGB		0x00004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_YUV420T	0x20017 /* 00_00_0_10_1_11 */
#define GE2D_FMT_M24_YUV420B	0x2001f /* 00_00_0_11_1_11 */

#define GE2D_FMT_M24_YUV420SP		0x20207
/* 01_00_0_00_1_11 nv12 &nv21, only works on m6. */
#define GE2D_FMT_M24_YUV420SPT		0x20217
/* 01_00_0_00_1_11 nv12 &nv21, only works on m6. */
#define GE2D_FMT_M24_YUV420SPB		0x2021f

#define GE2D_FMT_S16_YUV422T	0x20110 /* 01_00_0_10_0_00 */
#define GE2D_FMT_S16_YUV422B	0x20138 /* 01_00_0_11_0_00 */
#define GE2D_FMT_S24_YUV444T	0x20210 /* 10_00_0_10_0_00 */
#define GE2D_FMT_S24_YUV444B	0x20218 /* 10_00_0_11_0_00 */

/* back compatible defines */
#define GE2D_FORMAT_S8_Y            (GE2D_FORMAT_YUV|GE2D_FMT_S8_Y)
#define GE2D_FORMAT_S8_CB          (GE2D_FORMAT_YUV|GE2D_FMT_S8_CB)
#define GE2D_FORMAT_S8_CR          (GE2D_FORMAT_YUV|GE2D_FMT_S8_CR)
#define GE2D_FORMAT_S8_R            GE2D_FMT_S8_R
#define GE2D_FORMAT_S8_G            GE2D_FMT_S8_G
#define GE2D_FORMAT_S8_B            GE2D_FMT_S8_B
#define GE2D_FORMAT_S8_A            GE2D_FMT_S8_A
#define GE2D_FORMAT_S8_LUT          GE2D_FMT_S8_LUT
/* nv12 &nv21, only works on m6. */
#define GE2D_FORMAT_M24_NV12  (GE2D_FMT_M24_YUV420SP | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_M24_NV12T (GE2D_FMT_M24_YUV420SPT | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_M24_NV12B (GE2D_FMT_M24_YUV420SPB | GE2D_COLOR_MAP_NV12)
#define GE2D_FORMAT_M24_NV21  (GE2D_FMT_M24_YUV420SP | GE2D_COLOR_MAP_NV21)
#define GE2D_FORMAT_M24_NV21T (GE2D_FMT_M24_YUV420SPT | GE2D_COLOR_MAP_NV21)
#define GE2D_FORMAT_M24_NV21B (GE2D_FMT_M24_YUV420SPB | GE2D_COLOR_MAP_NV21)


#define GE2D_FORMAT_S12_RGB_655 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGB655)
#define GE2D_FORMAT_S16_YUV422 (GE2D_FMT_S16_YUV422 | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_RGB_655 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGB655)
#define GE2D_FORMAT_S24_YUV444 (GE2D_FMT_S24_YUV444 | GE2D_COLOR_MAP_YUV444)
#define GE2D_FORMAT_S24_RGB (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGB888)
#define GE2D_FORMAT_S32_YUVA444 (GE2D_FMT_S32_YUVA444 | GE2D_COLOR_MAP_YUVA4444)
#define GE2D_FORMAT_S32_RGBA (GE2D_FMT_S32_RGBA | GE2D_COLOR_MAP_RGBA8888)
#define GE2D_FORMAT_M24_YUV420      GE2D_FMT_M24_YUV420
#define GE2D_FORMAT_M24_YUV422      GE2D_FMT_M24_YUV422
#define GE2D_FORMAT_M24_YUV444      GE2D_FMT_M24_YUV444
#define GE2D_FORMAT_M24_RGB         GE2D_FMT_M24_RGB
#define GE2D_FORMAT_M24_YUV420T     GE2D_FMT_M24_YUV420T
#define GE2D_FORMAT_M24_YUV420B     GE2D_FMT_M24_YUV420B
#define GE2D_FORMAT_S16_YUV422T (GE2D_FMT_S16_YUV422T | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_YUV422B (GE2D_FMT_S16_YUV422B | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S24_YUV444T (GE2D_FMT_S24_YUV444T | GE2D_COLOR_MAP_YUV444)
#define GE2D_FORMAT_S24_YUV444B (GE2D_FMT_S24_YUV444B | GE2D_COLOR_MAP_YUV444)
/* format added in A1H */
/*16 bit*/
#define GE2D_FORMAT_S16_RGB_565   (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGB565)
#define GE2D_FORMAT_S16_RGB_844   (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGB844)
#define GE2D_FORMAT_S16_RGBA_6442 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGBA6442)
#define GE2D_FORMAT_S16_RGBA_4444 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGBA4444)
#define GE2D_FORMAT_S16_ARGB_4444 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_ARGB4444)
#define GE2D_FORMAT_S16_ARGB_1555 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_ARGB1555)
#define GE2D_FORMAT_S16_RGBA_4642 (GE2D_FMT_S16_RGB | GE2D_COLOR_MAP_RGBA4642)
/*24 bit*/
#define GE2D_FORMAT_S24_RGBA_5658 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGBA5658)
#define GE2D_FORMAT_S24_ARGB_8565 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_ARGB8565)
#define GE2D_FORMAT_S24_RGBA_6666 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_RGBA6666)
#define GE2D_FORMAT_S24_ARGB_6666 (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_ARGB6666)
#define GE2D_FORMAT_S24_BGR (GE2D_FMT_S24_RGB | GE2D_COLOR_MAP_BGR888)
/*32 bit*/
#define GE2D_FORMAT_S32_ARGB (GE2D_FMT_S32_RGBA | GE2D_COLOR_MAP_ARGB8888)
#define GE2D_FORMAT_S32_ABGR (GE2D_FMT_S32_RGBA | GE2D_COLOR_MAP_ABGR8888)
#define GE2D_FORMAT_S32_BGRA (GE2D_FMT_S32_RGBA | GE2D_COLOR_MAP_BGRA8888)


#define	UPDATE_SRC_DATA     0x01
#define	UPDATE_SRC_GEN      0x02
#define	UPDATE_DST_DATA     0x04
#define	UPDATE_DST_GEN      0x08
#define	UPDATE_DP_GEN       0x10
#define	UPDATE_SCALE_COEF   0x20
#define	UPDATE_ALL          0x3f

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

//struct ge2d_gen_s {
//	unsigned char     interrupt_ctrl;
//
//	unsigned char     dp_onoff_mode;
//	unsigned char     vfmt_onoff_en;
//	unsigned int      dp_on_cnt;
//	unsigned int      dp_off_cnt;
//};
//
//struct ge2d_src1_data_s {
//	unsigned char     urgent_en;
//	unsigned char     ddr_burst_size_y;
//	unsigned char     ddr_burst_size_cb;
//	unsigned char     ddr_burst_size_cr;
//	unsigned int	  canaddr;
//	unsigned char     x_yc_ratio;
//	unsigned char     y_yc_ratio;
//	unsigned char     sep_en;
//	unsigned char     format;
//
//	unsigned char     endian;
//	unsigned char     color_map;
//
//	unsigned char     mode_8b_sel;
//	unsigned char     lut_en;
//	unsigned int      def_color;
//	unsigned int      format_all;
//};
//
//struct ge2d_src1_gen_s {
//	int               clipx_start;
//	int               clipx_end;
//	int               clipy_start;
//	int               clipy_end;
//	unsigned char     clipx_start_ex;
//	unsigned char     clipx_end_ex;
//	unsigned char     clipy_start_ex;
//	unsigned char     clipy_end_ex;
//	unsigned char     pic_struct;
//	/* bit1 for outside alpha , bit0 for color data */
//	unsigned char     fill_mode;
//	unsigned int      outside_alpha;
//	unsigned char     chfmt_rpt_pix;
//	unsigned char     cvfmt_rpt_pix;
//};
//
//struct ge2d_src2_dst_data_s {
//	unsigned char     urgent_en;
//	unsigned char     ddr_burst_size;
//	unsigned char     src2_canaddr;
//	unsigned char     src2_format;
//
//	unsigned char     src2_endian;
//	unsigned char     src2_color_map;
//
//	unsigned char     src2_mode_8b_sel;
//	unsigned int      src2_def_color;
//
//	unsigned int     dst_canaddr;
//	unsigned char    dst_format;
//
//	unsigned char     dst_endian;
//	unsigned char     dst_color_map;
//
//	unsigned char     dst_mode_8b_sel;
//
//	unsigned int      src2_format_all;
//	unsigned int      dst_format_all;
//
//	/* only for m6 */
//	unsigned char	dst2_pixel_byte_width;
//	unsigned char	dst2_color_map;
//	unsigned char	dst2_discard_mode;
//	unsigned char	dst2_enable;
//
//};
//
//struct ge2d_src2_dst_gen_s {
//	int               src2_clipx_start;
//	int               src2_clipx_end;
//	int               src2_clipy_start;
//	int               src2_clipy_end;
//	unsigned char     src2_pic_struct;
//	/* bit1 for outside alpha , bit0 for color data */
//	unsigned char     src2_fill_mode;
//	unsigned int      src2_outside_alpha;
//
//	int               dst_clipx_start;
//	int               dst_clipx_end;
//	int               dst_clipy_start;
//	int               dst_clipy_end;
//	unsigned char     dst_clip_mode;
//	unsigned char     dst_pic_struct;
//};
//
//struct ge2d_dp_gen_s {
//	/* scaler related */
//	unsigned char     src1_vsc_bank_length;
//	unsigned char     src1_vsc_phase0_always_en;
//	unsigned char     src1_hsc_bank_length;
//	unsigned char     src1_hsc_phase0_always_en;
//	/* 1bit, 0: using minus, 1: using repeat data */
//	unsigned char     src1_hsc_rpt_ctrl;
//	/* 1bit, 0: using minus  1: using repeat data */
//	unsigned char     src1_vsc_rpt_ctrl;
//	unsigned char     src1_hsc_nearest_en;
//	unsigned char     src1_vsc_nearest_en;
//
//	unsigned char     antiflick_en;
//	unsigned char     antiflick_ycbcr_rgb_sel;
//	unsigned char     antiflick_cbcr_en;
//	/* Y= (R * r_coef + G * g_coef + B * b_coef)/256 */
//	unsigned int      antiflick_r_coef;
//	unsigned int      antiflick_g_coef;
//	unsigned int      antiflick_b_coef;
//	unsigned int      antiflick_color_filter_n1[4];
//	unsigned int      antiflick_color_filter_n2[4];
//	unsigned int      antiflick_color_filter_n3[4];
//	unsigned int      antiflick_color_filter_th[3];
//	unsigned int      antiflick_alpha_filter_n1[4];
//	unsigned int      antiflick_alpha_filter_n2[4];
//	unsigned int      antiflick_alpha_filter_n3[4];
//	unsigned int      antiflick_alpha_filter_th[3];
//	/* matrix related */
//	unsigned char     use_matrix_default;
//	unsigned char     conv_matrix_en;
//	unsigned char     matrix_sat_in_en;
//	unsigned char     matrix_minus_16_ctrl; /* 3bit */
//	unsigned char     matrix_sign_ctrl;     /* 3bit */
//	int               matrix_offset[3];
//	int               matrix_coef[9];
//
//	unsigned char     src1_gb_alpha;
//	unsigned int      alu_const_color;
//
//	unsigned char     src1_key_en;
//	unsigned char     src2_key_en;
//	unsigned char     src1_key_mode;
//	unsigned char     src2_key_mode;
//	unsigned int      src1_key;
//	unsigned int      src2_key;
//	unsigned int      src1_key_mask;
//	unsigned int      src2_key_mask;
//	unsigned char     bitmask_en;
//	unsigned char     bytemask_only;
//	unsigned int      bitmask;
//
//};

//struct ge2d_cmd_s {
//	int              src1_x_start;
//	int              src1_y_start;
//	int              src1_x_end;
//	int              src1_y_end;
//	/* unsigned char    src1_x_start_ex; */
//	/* unsigned char    src1_y_start_ex; */
//	/* unsigned char    src1_x_end_ex; */
//	/* unsigned char    src1_y_end_ex; */
//
//	unsigned char    src1_x_rev;
//	unsigned char    src1_y_rev;
//	/* unsigned char    src1_x_chr_phase; */
//	/* unsigned char    src1_y_chr_phase; */
//	unsigned char    src1_fill_color_en;
//
//	int              src2_x_start;
//	int              src2_y_start;
//	int              src2_x_end;
//	int              src2_y_end;
//	unsigned char    src2_x_rev;
//	unsigned char    src2_y_rev;
//	unsigned char    src2_fill_color_en;
//
//	int              dst_x_start;
//	int              dst_y_start;
//	int              dst_x_end;
//	int              dst_y_end;
//	unsigned char    dst_xy_swap;
//	unsigned char    dst_x_rev;
//	unsigned char    dst_y_rev;
//
//	int              sc_prehsc_en;
//	int              sc_prevsc_en;
//	int              sc_hsc_en;
//	int              sc_vsc_en;
//	int              vsc_phase_step;
//	int              vsc_phase_slope;
//	unsigned char    vsc_rpt_l0_num;
//	int              vsc_ini_phase;
//	int              hsc_phase_step;
//	int              hsc_phase_slope;
//	unsigned char    hsc_rpt_p0_num;
//	int              hsc_ini_phase;
//	unsigned char    hsc_div_en;
//	unsigned int    hsc_div_length;
//	int              hsc_adv_num;
//	int              hsc_adv_phase;
//
//	unsigned char    src1_cmult_asel;
//	unsigned char    src2_cmult_asel;
//
//	unsigned char    color_blend_mode;
//	unsigned char    color_src_blend_factor;
//	unsigned char    color_dst_blend_factor;
//	unsigned char    color_logic_op;
//
//	unsigned char    alpha_blend_mode;
//	unsigned char    alpha_src_blend_factor;
//	unsigned char    alpha_dst_blend_factor;
//	unsigned char    alpha_logic_op;
//
//	int (*cmd_cb)(unsigned);
//	unsigned int     cmd_cb_param;
//	unsigned int     src1_buffer;
//	unsigned int     src2_buffer;
//	unsigned char    release_flag;
//	unsigned char    wait_done_flag;
//};

//struct ge2d_config_s {
//	struct ge2d_gen_s            gen;
//	struct ge2d_src1_data_s      src1_data;
//	struct ge2d_src1_gen_s       src1_gen;
//	struct ge2d_src2_dst_data_s  src2_dst_data;
//	struct ge2d_src2_dst_gen_s   src2_dst_gen;
//	struct ge2d_dp_gen_s         dp_gen;
//	unsigned              v_scale_coef_type;
//	unsigned              h_scale_coef_type;
//	unsigned              update_flag;
//};

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

//struct ge2d_queue_item_s {
//	struct list_head list;
//	struct ge2d_cmd_s cmd;
//	struct ge2d_config_s config;
//};

//struct ge2d_context_s {
//	/* connect all process in one queue for RR process. */
//	struct list_head   list;
//	/* current wq configuration */
//	struct ge2d_config_s       config;
//	struct ge2d_cmd_s		cmd;
//	struct list_head	work_queue;
//	struct list_head	free_queue;
//	wait_queue_head_t	cmd_complete;
//	int				queue_dirty;
//	int				queue_need_recycle;
//	spinlock_t		lock;	/* for get and release item. */
//};
//
//struct ge2d_event_s {
//	wait_queue_head_t cmd_complete;
//	struct completion process_complete;
//	/* for queue switch and create destroy queue. */
//	spinlock_t sem_lock;
//	struct semaphore cmd_in_sem;
//};
//
//struct ge2d_manager_s {
//	struct list_head process_queue;
//	struct ge2d_context_s *current_wq;
//	struct ge2d_context_s *last_wq;
//	struct task_struct *ge2d_thread;
//	struct ge2d_event_s event;
//	int irq_num;
//	int ge2d_state;
//	int process_queue_state;
//	struct platform_device *pdev;
//};

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

#endif

