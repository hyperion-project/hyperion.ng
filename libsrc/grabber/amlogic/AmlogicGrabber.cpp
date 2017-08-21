// STL includes
#include <algorithm>
#include <cassert>
#include <iostream>
#include <QFile>

// Linux includes
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

// Local includes
#include <utils/Logger.h>
#include <grabber/AmlogicGrabber.h>
#include "Amvideocap.h"

#include "ge2d.h"
#include "ge2d_cmd.h"
//#include "ge2d_func.h"


#define VIDEO_DEVICE   "/dev/amvideo"
#define CAPTURE_DEVICE "/dev/amvideocap0"
#define GE2D_DEVICE    "/dev/ge2d"

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height)
	: Grabber("AMLOGICGRABBER", qMax(285u, width), qMax(160u, height)) // Minimum required width or height is 160
	, _captureDev(-1)
	, _videoDev(-1)
	, _ge2dDev(-1)
	, _lastError(0)
	, _fbGrabber("/dev/fb0",width,height)
	, _ge2dAvailable(true)
	, _grabbingModeNotification(0)
{
	Debug(_log, "constructed(%d x %d)",_width,_height);
}

AmlogicGrabber::~AmlogicGrabber()
{
	if (_captureDev != -1) close(_captureDev);
	if (_videoDev   != -1) close(_videoDev);
	if (_ge2dDev    != -1) close(_ge2dDev);
}


bool AmlogicGrabber::isVideoPlaying()
{
	if(!QFile::exists(_videoDevice)) return false;

	int videoDisabled = 0;
	if (_videoDev<0)
		_videoDev = open(VIDEO_DEVICE, O_RDWR);

	if (_videoDev < 0)
	{
		Error(_log, "Failed to open video device(%s): %d - %s", VIDEO_DEVICE, errno, strerror(errno));
		videoDisabled = 1;
	}
	else
	{
		// Check the video disabled flag
		if(ioctl(_videoDev, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) == -1)
		{
			Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
			videoDisabled = 1;
			close(_videoDev);
			_videoDev = -1;
		}
	}

	return videoDisabled == 0;
}

int AmlogicGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	// Make sure video is playing, else there is nothing to grab
	int ret = -1;
	if (isVideoPlaying())
	{
		InfoIf(_grabbingModeNotification!=1, _log, "VPU mode");
		_grabbingModeNotification = 1;
		if (_ge2dAvailable)
		{
			ret = grabFrame_ge2d(image);
			_ge2dAvailable = (ret == 0);
			WarningIf(!_ge2dAvailable, _log, "GE2D capture interface not available! try Amvideocap instead");
		}
		else if (QFile::exists(_captureDevice))
		{
			ret = grabFrame_amvideocap(image);
		}
	}
	else
	{
		InfoIf(_grabbingModeNotification!=2, _log, "FB mode");
		_grabbingModeNotification = 2;
		ret = _fbGrabber.grabFrame(image);
	}
	
	return 0;
}


int AmlogicGrabber::grabFrame_amvideocap(Image<ColorRgb> & image)
{
	// If the device is not open, attempt to open it
	if (_captureDev < 0)
	{
		_captureDev = open(CAPTURE_DEVICE, O_RDWR);

		// If the device is still not open, there is something wrong
		if (_captureDev < 0)
		{
			ErrorIf( _lastError != 1, _log,"Failed to open the AMLOGIC device (%d - %s):", errno, strerror(errno));
			_lastError = 1;
			return -1;
		}
	}

	int w=_width,h=_height;
	bool ret = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,  w)  < 0 || ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, h) < 0;
	if (!ret || w==0 || h==0)
	{
		// Failed to configure frame width
		ErrorIf(_lastError != 2,_log,"Failed to configure capture size (%d - %s)", errno, strerror(errno));
		close(_captureDev);
		_lastError = 2;
		_captureDev = -1;
		return -1;
	}

	// Read the snapshot into the memory
	void * image_ptr = _image.memptr();
	const ssize_t bytesToRead = _width * _height * sizeof(ColorBgr);

	const ssize_t bytesRead   = pread(_captureDev, image_ptr, bytesToRead, 0);
	if (bytesRead < 0)
	{
		ErrorIf(_lastError != 3, _log,"Read of device failed: %d - %s", errno, strerror(errno));
		close(_captureDev);
		_lastError = 3;
		_captureDev = -1;
		return -1;
	}
	else if (bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		ErrorIf(_lastError != 4, _log,"Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", bytesToRead, bytesRead);
		close(_captureDev);
		_captureDev = -1;
		return -1;
	}

	// For now we always close the device now and again
// 	static int readCnt = 0;
// 	++readCnt;
// 	if (readCnt > 20)
// 	{
// 		close(_captureDev);
// 		_captureDev = -1;
// 		readCnt = 0;
// 	}
	
	//_imageResampler.setHorizontalPixelDecimation(w/_width);
	//_imageResampler.setVerticalPixelDecimation(h/_height);
	_imageResampler.processImage((const uint8_t*)image_ptr, _width, _height, 3, PIXELFORMAT_BGR24, image);

	_lastError = 0;
	return 0;
}

int AmlogicGrabber::grabFrame_ge2d(Image<ColorRgb> & image)
{
	if (_ge2dDev<0)
		_ge2dDev = open(GE2D_DEVICE, O_RDWR);
		
	// Ion
	IonBuffer videoBuffer(_width * _height * 3); // RGB565

	void* videoBufferPtr = videoBuffer.Map();
	memset(videoBufferPtr, 0, videoBuffer.BufferSize());



	struct config_para_ex_s configex = { 0 };
	ge2d_para_s blitRect = { 0 };

	int canvas_index;
	if (ioctl(_videoDev, AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME, &canvas_index) < 0)
	{
		Error(_log, "AMSTREAM_EXT_GET_CURRENT_VIDEOFRAME failed.");
		return -1;
	}

	uint32_t canvas0addr;

	if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR, &canvas0addr) < 0)
	{
		Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR failed.");
		return -1;
	}
	//printf("amvideo: canvas0addr=%x\n", canvas0addr);

	uint32_t ge2dformat;
	if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT, &ge2dformat) <0)
	{
		Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT failed.");
		return -1;
	}
	//printf("amvideo: ge2dformat=%x\n", ge2dformat);

	uint64_t size;
	if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_SIZE, &size) < 0)
	{
		Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_SIZE failed.");
		return -1;
	}

	int videoWidth = size >> 32;
	int videoHeight = size & 0xffffff;
	//printf("amvideo: size=%x (%dx%d)\n", size, size >> 32, size & 0xffffff);

	Rectangle videoRect;
	float videoAspect = (float)videoWidth / (float)videoHeight;

	configex = { 0 };
	configex.src_para.mem_type = CANVAS_TYPE_INVALID;
	configex.src_para.canvas_index = canvas0addr;
	configex.src_para.left = 0;
	configex.src_para.top = 0;
	configex.src_para.width = videoWidth;
	configex.src_para.height = videoHeight / 2;
	configex.src_para.format = ge2dformat;

	configex.dst_para.mem_type = CANVAS_ALLOC;
	configex.dst_para.format = GE2D_FORMAT_S24_RGB;
	configex.dst_para.left = 0;
	configex.dst_para.top = 0;

	configex.dst_para.width = _width;
	configex.dst_para.height = _height;
	configex.dst_planes[0].addr = (long unsigned int)videoBuffer.PhysicalAddress();
	configex.dst_planes[0].w = configex.dst_para.width;
	configex.dst_planes[0].h = configex.dst_para.height;

	if (ioctl(_ge2dDev, GE2D_CONFIG_EX, &configex) < 0)
	{
		Error(_log, "video GE2D_CONFIG_EX failed.");
		return -1;
	}

	
	// Return video frame
	if (ioctl(_videoDev, AMVIDEO_EXT_PUT_CURRENT_VIDEOFRAME) < 0)
	{
		Error(_log, "AMSTREAM_EXT_PUT_CURRENT_VIDEOFRAME failed.");
		return -1;
	}

	return 0;
}




/*
#define VIDEOCAPDEV "/dev/amvideocap0"              
int amvideocap_capframe(char *buf,int size,int *w,int *h,int fmt_ignored,int at_end, int* ret_size)
{
	int fd=open(VIDEOCAPDEV,O_RDWR);
	int ret = 0;
	if(fd<0){
		ALOGI("amvideocap_capframe open %s failed\n",VIDEOCAPDEV);
		return -1;
	}
	if(w!= NULL && *w>0){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,*w);
	}
	if(h!= NULL && *h>0){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT,*h);
	}
	if(at_end){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS,CAP_FLAG_AT_END);
	}
	*ret_size =read(fd,buf,size);
	if(w != NULL){
		ret=ioctl(fd,AMVIDEOCAP_IOR_GET_FRAME_WIDTH,w);
	}
	if(h != NULL){
		ret=ioctl(fd,AMVIDEOCAP_IOR_GET_FRAME_HEIGHT,h);
	}
	close(fd);
	return ret;
}

int amvideocap_capframe_with_rect(char *buf,int size, int src_rect_x, int src_rect_y, int *w,int *h,int fmt_ignored,int at_end, int* ret_size)
{
	int fd=open(VIDEOCAPDEV,O_RDWR);
	int ret = 0;
	if(fd<0){
		ALOGI("amvideocap_capframe_with_rect open %s failed\n",VIDEOCAPDEV);
		return -1;
	}
	ALOGI("amvideocap_capframe_with_rect open %d, %d\n",*w, *h);
    if(src_rect_x>0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_SET_SRC_X,src_rect_x);
	}
    if(src_rect_y>0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_SET_SRC_Y,src_rect_y);
	}
    if(*w>0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_SET_SRC_WIDTH,*w);
	}
    if(*h>0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_SET_SRC_HEIGHT,*h);
	}
	if(*w>0){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,*w);
	}
	if(*h>0){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT,*h);
	}
	if(at_end){
		ret=ioctl(fd,AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS,CAP_FLAG_AT_END);
	}
	*ret_size =read(fd,buf,size);

	if(*w>0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_GET_FRAME_WIDTH,w);
	}

	if(*h>0){
		ret=ioctl(fd,AMVIDEOCAP_IOR_GET_FRAME_HEIGHT,h);
	}
	close(fd);
	return ret;
}

 */