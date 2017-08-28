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

#define VIDEO_DEVICE   "/dev/amvideo"
#define CAPTURE_DEVICE "/dev/amvideocap0"

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height)
	: Grabber("AMLOGICGRABBER", qMax(285u, width), qMax(160u, height)) // Minimum required width or height is 160
	, _captureDev(-1)
	, _videoDev(-1)
	, _lastError(0)
	, _fbGrabber("/dev/fb0",width,height)
	, _grabbingModeNotification(0)
{
	Debug(_log, "constructed(%d x %d)",_width,_height);
}

AmlogicGrabber::~AmlogicGrabber()
{
	if (_captureDev != -1) close(_captureDev);
	if (_videoDev   != -1) close(_videoDev);
}


bool AmlogicGrabber::isVideoPlaying()
{
	if(!QFile::exists(VIDEO_DEVICE)) return false;

	int videoDisabled = 1;
	if (_videoDev<0)
		_videoDev = open(VIDEO_DEVICE, O_RDWR);

	if (_videoDev < 0)
	{
		Error(_log, "Failed to open video device(%s): %d - %s", VIDEO_DEVICE, errno, strerror(errno));
		return false;
	}
	else
	{
		// Check the video disabled flag
		if(ioctl(_videoDev, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) == -1)
		{
			Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
			close(_videoDev);
			_videoDev = -1;
			return false;
		}
	}

	return videoDisabled == 0;
}

int AmlogicGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	if (image.width()!=(unsigned)_width || image.height()!=(unsigned)_height)
		image.resize(_width, _height);

	// Make sure video is playing, else there is nothing to grab
	if (isVideoPlaying())
	{
		if (_grabbingModeNotification!=1)
		{
			Info(_log, "VPU mode");
			_grabbingModeNotification = 1;
			_lastError = 0;
		}

		if (QFile::exists(CAPTURE_DEVICE))
		{
			grabFrame_amvideocap(image);
		}
	}
	else
	{
		if (_grabbingModeNotification!=2)
		{
			Info( _log, "FB mode");
			_grabbingModeNotification = 2;
			_lastError = 0;
		}
		_fbGrabber.grabFrame(image);
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
	_image.resize(w,h);
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

	_imageResampler.setHorizontalPixelDecimation(1);
	_imageResampler.setVerticalPixelDecimation(1);
	_imageResampler.processImage((const uint8_t*)image_ptr, _width, _height, 3, PIXELFORMAT_BGR24, image);
	_lastError = 0;

	return 0;
}

