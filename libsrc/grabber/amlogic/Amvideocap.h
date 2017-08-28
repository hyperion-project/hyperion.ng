#pragma once

#include <exception>
#include <linux/videodev2.h>

#define AMVIDEOCAP_IOC_MAGIC  'V'
#define CAP_FLAG_AT_CURRENT		0
#define CAP_FLAG_AT_TIME_WINDOW	1
#define CAP_FLAG_AT_END			2

/*
format see linux/ge2d/ge2d.h
like:
GE2D_FORMAT_S24_RGB
*/

#define u64 unsigned long

#define AMVIDEOCAP_IOW_SET_WANTFRAME_FORMAT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x01, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH      		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_TIMESTAMP_MS     	_IOW(AMVIDEOCAP_IOC_MAGIC, 0x04, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WAIT_MAX_MS     	_IOW(AMVIDEOCAP_IOC_MAGIC, 0x05, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x06, u64)


#define AMVIDEOCAP_IOR_GET_FRAME_FORMAT     		_IOR(AMVIDEOCAP_IOC_MAGIC, 0x10, u64)
#define AMVIDEOCAP_IOR_GET_FRAME_WIDTH      		_IOR(AMVIDEOCAP_IOC_MAGIC, 0x11, u64)
#define AMVIDEOCAP_IOR_GET_FRAME_HEIGHT     		_IOR(AMVIDEOCAP_IOC_MAGIC, 0x12, u64)
#define AMVIDEOCAP_IOR_GET_FRAME_TIMESTAMP_MS     	_IOR(AMVIDEOCAP_IOC_MAGIC, 0x13, u64)


#define AMVIDEOCAP_IOR_GET_SRCFRAME_FORMAT      			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x20, u64)
#define AMVIDEOCAP_IOR_GET_SRCFRAME_WIDTH       			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x21, u64)
#define AMVIDEOCAP_IOR_GET_SRCFRAME_HEIGHT      			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x22, u64)


#define AMVIDEOCAP_IOR_GET_STATE     	   			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x31, u64)
#define AMVIDEOCAP_IOW_SET_START_CAPTURE   			_IOW(AMVIDEOCAP_IOC_MAGIC, 0x32, u64)
#define AMVIDEOCAP_IOW_SET_CANCEL_CAPTURE  			_IOW(AMVIDEOCAP_IOC_MAGIC, 0x33, u64)

#define AMVIDEOCAP_IOR_SET_SRC_X                _IOR(AMVIDEOCAP_IOC_MAGIC, 0x40, u64)
#define AMVIDEOCAP_IOR_SET_SRC_Y                _IOR(AMVIDEOCAP_IOC_MAGIC, 0x41, u64)
#define AMVIDEOCAP_IOR_SET_SRC_WIDTH            _IOR(AMVIDEOCAP_IOC_MAGIC, 0x42, u64)
#define AMVIDEOCAP_IOR_SET_SRC_HEIGHT           _IOR(AMVIDEOCAP_IOC_MAGIC, 0x43, u64)

enum amvideocap_state{
	AMVIDEOCAP_STATE_INIT=0,
	AMVIDEOCAP_STATE_ON_CAPTURE=200,
	AMVIDEOCAP_STATE_FINISHED_CAPTURE=300,
	AMVIDEOCAP_STATE_ERROR=0xffff,
};


#define AMVIDEO_MAGIC  'X'

#define AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME _IOR((AMVIDEO_MAGIC), 0x01, int32_t)
#define AMVIDEO_EXT_PUT_CURRENT_VIDEOFRAME _IO((AMVIDEO_MAGIC), 0x02)

#define AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT _IOR((AMVIDEO_MAGIC), 0x03, uint32_t)
#define AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_SIZE _IOR((AMVIDEO_MAGIC), 0x04, uint64_t)
#define AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR _IOR((AMVIDEO_MAGIC), 0x05, uint32_t)

#define _A_M  'S'
#define AMSTREAM_IOC_GET_VIDEO_DISABLE  _IOR((_A_M), 0x48, int)
#define AMSTREAM_IOC_SET_VIDEO_DISABLE  _IOW((_A_M), 0x49, int)
struct Rectangle
{
	int X;
	int Y;
	int Width;
	int Height;
};
