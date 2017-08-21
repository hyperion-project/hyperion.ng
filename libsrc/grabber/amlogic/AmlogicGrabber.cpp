/*
mpeg12
mpeg4
vc1
h264
h264mvc
hmvc
h264_4k2k
h265
hevc
mjpeg
real
avs
*/

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

#define _A_M  'S'
#define AMSTREAM_IOC_GET_VIDEO_DISABLE  _IOR((_A_M), 0x48, int)
#define AMSTREAM_IOC_SET_VIDEO_DISABLE  _IOW((_A_M), 0x49, int)

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height)
	: Grabber("AMLOGICGRABBER", qMax(285u, width), qMax(160u, height)) // Minimum required width or height is 160
	, _amlogicCaptureDev(-1)
	, _videoDevice("/dev/amvideo")
	, _captureDevice("/dev/amvideocap0")
	, _lastError(0)
	, _fbGrabber("/dev/fb0",width,height)
	, _ge2dAvailable(true)
	, _grabbingModeNotification(0)
{
	Debug(_log, "constructed(%d x %d)",_width,_height);
}

AmlogicGrabber::~AmlogicGrabber()
{
	if (_amlogicCaptureDev != -1)
	{
		if (close(_amlogicCaptureDev) == -1)
		{
			ErrorIf(_lastError != 5, _log, "Failed to close device (%d - %s)", errno, strerror(errno));
			_lastError = 5;
		}
		_amlogicCaptureDev = -1;
	}
}


bool AmlogicGrabber::isVideoPlaying()
{
	if(!QFile::exists(_videoDevice)) return false;

	int videoDisabled = 0;
	int video_fd = open(QSTRING_CSTR(_videoDevice), O_RDONLY);
	if (video_fd < 0)
	{
		Error(_log, "Failed to open video device(%s): %d - %s", QSTRING_CSTR(_videoDevice), errno, strerror(errno));
		videoDisabled = 1;
	}
	else
	{
		// Check the video disabled flag
		if(ioctl(video_fd, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) == -1)
		{
			Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
			videoDisabled = 1;
		}
		close(video_fd);
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
	if (_amlogicCaptureDev == -1)
	{
		_amlogicCaptureDev = open(QSTRING_CSTR(_captureDevice), O_RDWR);

		// If the device is still not open, there is something wrong
		if (_amlogicCaptureDev == -1)
		{
			ErrorIf( _lastError != 1, _log,"Failed to open the AMLOGIC device (%d - %s):", errno, strerror(errno));
			_lastError = 1;
			return -1;
		}
	}

	int w=_width,h=_height;
	bool ret = ioctl(_amlogicCaptureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,  w)  == -1 || ioctl(_amlogicCaptureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, h) == -1;
	if (!ret || w==0 || h==0)
	{
		// Failed to configure frame width
		ErrorIf(_lastError != 2,_log,"Failed to configure capture size (%d - %s)", errno, strerror(errno));
		close(_amlogicCaptureDev);
		_lastError = 2;
		_amlogicCaptureDev = -1;
		return -1;
	}

	// Read the snapshot into the memory
	void * image_ptr = _image.memptr();
	const ssize_t bytesToRead = _width * _height * sizeof(ColorBgr);

	const ssize_t bytesRead   = pread(_amlogicCaptureDev, image_ptr, bytesToRead, 0);
	if (bytesRead == -1)
	{
		ErrorIf(_lastError != 3, _log,"Read of device failed: %d - %s", errno, strerror(errno));
		close(_amlogicCaptureDev);
		_lastError = 3;
		_amlogicCaptureDev = -1;
		return -1;
	}
	else if (bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		ErrorIf(_lastError != 4, _log,"Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", bytesToRead, bytesRead);
		close(_amlogicCaptureDev);
		_amlogicCaptureDev = -1;
		return -1;
	}

	// For now we always close the device now and again
	static int readCnt = 0;
	++readCnt;
	if (readCnt > 20)
	{
		close(_amlogicCaptureDev);
		_amlogicCaptureDev = -1;
		readCnt = 0;
	}
	
	_imageResampler.setHorizontalPixelDecimation(w/_width);
	_imageResampler.setVerticalPixelDecimation(h/_height);
	_imageResampler.processImage((const uint8_t*)image_ptr, _width, _height, 3, PIXELFORMAT_BGR24, image);

	_lastError = 0;
	return 0;
}

int AmlogicGrabber::grabFrame_ge2d(Image<ColorRgb> & image)
{
	return -1;
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