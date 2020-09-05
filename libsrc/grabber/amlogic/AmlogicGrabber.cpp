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

AmlogicGrabber::AmlogicGrabber(unsigned width, unsigned height)
	: Grabber("AMLOGICGRABBER", qMax(160u, width), qMax(160u, height)) // Minimum required width or height is 160
	, _captureDev(-1)
	, _videoDev(-1)
	, _lastError(0)
	, _fbGrabber("/dev/fb0",width,height)
	, _grabbingModeNotification(0)
{
	Debug(_log, "constructed(%d x %d), grabber device: %s",_width,_height, CAPTURE_DEVICE);

	_image_bgr.resize(_width, _height);
	_bytesToRead = _image_bgr.size();
	_image_ptr = _image_bgr.memptr();
}

AmlogicGrabber::~AmlogicGrabber()
{
	closeDev(_captureDev);
	closeDev(_videoDev);
}

bool AmlogicGrabber::openDev(int &fd, const char* dev)
{
	if (fd<0)
	{
		fd = open(dev, O_RDWR);
	}
	return fd >= 0;
}

void AmlogicGrabber::closeDev(int &fd)
{
	if (fd >= 0)
	{
		close(fd);
		fd = -1;
	}
}

bool AmlogicGrabber::isVideoPlaying()
{
	if(!QFile::exists(VIDEO_DEVICE)) return false;

	int videoDisabled = 1;
	if (!openDev(_videoDev, VIDEO_DEVICE))
	{
		Error(_log, "Failed to open video device(%s): %d - %s", VIDEO_DEVICE, errno, strerror(errno));
		return false;
	}
	else
	{
		// Check the video disabled flag
		if(ioctl(_videoDev, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) < 0)
		{
			Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
			closeDev(_videoDev);
			return false;
		}
	}

	return videoDisabled == 0;
}

int AmlogicGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	// Make sure video is playing, else there is nothing to grab
	if (isVideoPlaying())
	{
		if (_grabbingModeNotification!=1)
		{
			Info(_log, "VPU mode");
			_grabbingModeNotification = 1;
			_lastError = 0;
		}

		if (grabFrame_amvideocap(image) < 0)
			closeDev(_captureDev);
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

		usleep(50 * 1000);
	}

	return 0;
}


int AmlogicGrabber::grabFrame_amvideocap(Image<ColorRgb> & image)
{
	// If the device is not open, attempt to open it
	if (_captureDev < 0)
	{
		if (! openDev(_captureDev, CAPTURE_DEVICE))
		{
			ErrorIf( _lastError != 1, _log,"Failed to open the AMLOGIC device (%d - %s):", errno, strerror(errno));
			_lastError = 1;
			return -1;
		}

		long r1 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, _width);
		long r2 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, _height);
		long r3 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS, CAP_FLAG_AT_END);

		if (r1<0 || r2<0 || r3<0 || _height==0 || _width==0)
		{
			ErrorIf(_lastError != 2,_log,"Failed to configure capture device (%d - %s)", errno, strerror(errno));
			_lastError = 2;
			return -1;
		}
	}

	// Read the snapshot into the memory
	ssize_t bytesRead   = pread(_captureDev, _image_ptr, _bytesToRead, 0);

	if (bytesRead < 0)
	{
		ErrorIf(_lastError != 3, _log,"Read of device failed: %d - %s", errno, strerror(errno));
		_lastError = 3;
		return -1;
	}
	else if (_bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		ErrorIf(_lastError != 4, _log,"Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", _bytesToRead, bytesRead);
		_lastError = 4;
		return -1;
	}

	_useImageResampler = true;
	_imageResampler.processImage((const uint8_t*)_image_ptr, _width, _height, (_width << 1) + _width, PixelFormat::BGR24, image);
	_lastError = 0;

	return 0;
}
