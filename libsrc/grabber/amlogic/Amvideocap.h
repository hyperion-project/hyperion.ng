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

// #define AMVIDEOCAP_IOW_SET_WANTFRAME_FORMAT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x01, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH      		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x06, int)

#define _A_M  'S'
#define AMSTREAM_IOC_GET_VIDEO_DISABLE  _IOR((_A_M), 0x48, int)
