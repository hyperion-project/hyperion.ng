#pragma once

#include <exception>
#include <linux/videodev2.h>
#include "ion.h"
#include "meson_ion.h"

#define AMVIDEOCAP_IOC_MAGIC  'V'
#define CAP_FLAG_AT_CURRENT		0
#define CAP_FLAG_AT_TIME_WINDOW	1
#define CAP_FLAG_AT_END			2

/*
format see linux/ge2d/ge2d.h
like:
GE2D_FORMAT_S24_RGB
*/

#define AMVIDEOCAP_IOW_SET_WANTFRAME_FORMAT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x01, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH      		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_TIMESTAMP_MS     	_IOW(AMVIDEOCAP_IOC_MAGIC, 0x04, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WAIT_MAX_MS     	_IOW(AMVIDEOCAP_IOC_MAGIC, 0x05, u64)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x06, int)


#define AMVIDEOCAP_IOR_GET_FRAME_FORMAT     		_IOR(AMVIDEOCAP_IOC_MAGIC, 0x10, int)
#define AMVIDEOCAP_IOR_GET_FRAME_WIDTH      		_IOR(AMVIDEOCAP_IOC_MAGIC, 0x11, int)
#define AMVIDEOCAP_IOR_GET_FRAME_HEIGHT     		_IOR(AMVIDEOCAP_IOC_MAGIC, 0x12, int)
#define AMVIDEOCAP_IOR_GET_FRAME_TIMESTAMP_MS     	_IOR(AMVIDEOCAP_IOC_MAGIC, 0x13, int)


#define AMVIDEOCAP_IOR_GET_SRCFRAME_FORMAT      			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x20, int)
#define AMVIDEOCAP_IOR_GET_SRCFRAME_WIDTH       			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x21, int)
#define AMVIDEOCAP_IOR_GET_SRCFRAME_HEIGHT      			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x22, int)


#define AMVIDEOCAP_IOR_GET_STATE     	   			_IOR(AMVIDEOCAP_IOC_MAGIC, 0x31, int)
#define AMVIDEOCAP_IOW_SET_START_CAPTURE   			_IOW(AMVIDEOCAP_IOC_MAGIC, 0x32, int)
#define AMVIDEOCAP_IOW_SET_CANCEL_CAPTURE  			_IOW(AMVIDEOCAP_IOC_MAGIC, 0x33, int)

#define AMVIDEOCAP_IOR_SET_SRC_X                _IOR(AMVIDEOCAP_IOC_MAGIC, 0x40, int)
#define AMVIDEOCAP_IOR_SET_SRC_Y                _IOR(AMVIDEOCAP_IOC_MAGIC, 0x41, int)
#define AMVIDEOCAP_IOR_SET_SRC_WIDTH            _IOR(AMVIDEOCAP_IOC_MAGIC, 0x42, int)
#define AMVIDEOCAP_IOR_SET_SRC_HEIGHT           _IOR(AMVIDEOCAP_IOC_MAGIC, 0x43, int)

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


struct IonBuffer
{
	ion_user_handle_t Handle;
	int ExportHandle;
	size_t Length;
	unsigned long PhysicalAddress;
};

IonBuffer IonAllocate(int ion_fd, size_t bufferSize)
{
	int io;
	IonBuffer result; 

	// Allocate a buffer
	ion_allocation_data allocation_data = { 0 };
	allocation_data.len = bufferSize;
	allocation_data.heap_id_mask = ION_HEAP_CARVEOUT_MASK;
	allocation_data.flags = ION_FLAG_CACHED;

	io = ioctl(ion_fd, ION_IOC_ALLOC, &allocation_data);
	if (io != 0)
	{
		throw std::Exception("ION_IOC_ALLOC failed.");
	}

	printf("ion handle=%d\n", allocation_data.handle);


	// Map/share the buffer
	ion_fd_data ionData = { 0 };
	ionData.handle = allocation_data.handle;

	io = ioctl(ion_fd, ION_IOC_SHARE, &ionData);
	if (io != 0)
	{
		throw std::Exception("ION_IOC_SHARE failed.");
	}

	printf("ion map=%d\n", ionData.fd);


	// Get the physical address for the buffer
	meson_phys_data physData = { 0 };
	physData.handle = ionData.fd;

	ion_custom_data ionCustomData = { 0 };
	ionCustomData.cmd = ION_IOC_MESON_PHYS_ADDR;
	ionCustomData.arg = (long unsigned int)&physData;

	io = ioctl(ion_fd, ION_IOC_CUSTOM, &ionCustomData);
	if (io != 0)
	{
		//throw Exception("ION_IOC_CUSTOM failed.");
		printf("ION_IOC_CUSTOM failed (%d).", io);
	}


	result.Handle = allocation_data.handle;
	result.ExportHandle = ionData.fd;
	result.Length = allocation_data.len;
	result.PhysicalAddress = physData.phys_addr;

	printf("ion phys_addr=%lu\n", result.PhysicalAddress);


	//ion_handle_data ionHandleData = { 0 };
	//ionHandleData.handle = allocation_data.handle;

	//io = ioctl(ion_fd, ION_IOC_FREE, &ionHandleData);
	//if (io != 0)
	//{
	//	throw Exception("ION_IOC_FREE failed.");
	//}

	return result;
}

