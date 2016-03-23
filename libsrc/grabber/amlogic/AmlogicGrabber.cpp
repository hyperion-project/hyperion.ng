
// STL includes
#include <algorithm>
#include <cassert>
#include <iostream>

// Linux includes
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

// Local includes
#include <grabber/AmlogicGrabber.h>

// Flags copied from 'include/linux/amlogic/amports/amvideocap.h' at https://github.com/codesnake/linux-amlogic
#define AMVIDEOCAP_IOC_MAGIC 'V'
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH  _IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT _IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)

// Flags copied from 'include/linux/amlogic/amports/amvstream.h' at https://github.com/codesnake/linux-amlogic
#define AMSTREAM_IOC_MAGIC   'S'
#define AMSTREAM_IOC_GET_VIDEO_DISABLE      _IOR(AMSTREAM_IOC_MAGIC,   0x48, unsigned long)

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height) :
	// Minimum required width or height is 160
	_width(std::max(160u, width)),
	_height(std::max(160u, height)),
	_amlogicCaptureDev(-1)
{
	std::cout << "AMLOGICGRABBER INFO: [" << __PRETTY_FUNCTION__ << "] constructed(" << _width << "x" << _height << ")" << std::endl;
}

AmlogicGrabber::~AmlogicGrabber()
{
	if (_amlogicCaptureDev != -1)
	{
		if (close(_amlogicCaptureDev) == -1)
		{
			std::cerr << "AMLOGICGRABBER ERROR: [" << __PRETTY_FUNCTION__ << "] Failed to close device (" << errno << ")" << std::endl;
		}
		_amlogicCaptureDev = -1;
	}
}

void AmlogicGrabber::setVideoMode(const VideoMode videoMode)
{
	switch (videoMode) {
	case VIDEO_3DSBS:
		//vc_dispmanx_rect_set(&_rectangle, 0, 0, _width/2, _height);
		break;
	case VIDEO_3DTAB:
		//vc_dispmanx_rect_set(&_rectangle, 0, 0, _width, _height/2);
		break;
	case VIDEO_2D:
	default:
		//vc_dispmanx_rect_set(&_rectangle, 0, 0, _width, _height);
		break;
	}
}

bool AmlogicGrabber::isVideoPlaying()
{
	const std::string videoDevice = "/dev/amvideo";

	// Open the video device
	int video_fd = open(videoDevice.c_str(), O_RDONLY);
	if (video_fd < 0)
	{
		std::cerr << "AMLOGICGRABBER ERROR: Failed to open video device(" << videoDevice << "): " << strerror(errno) << std::endl;
		return false;
	}

	// Check the video disabled flag
	int videoDisabled;
	if (ioctl(video_fd, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) == -1)
	{
		std::cerr << "AMLOGICGRABBER ERROR: Failed to retrieve video state from device: " << strerror(errno) << std::endl;
		close(video_fd);
		return false;
	}

	// Make sure to close the device after use
	close(video_fd);

	return videoDisabled == 0;
}

int AmlogicGrabber::grabFrame(Image<ColorBgr> & image)
{
	// resize the given image if needed
	if (image.width() != _width || image.height() != _height)
	{
		image.resize(_width, _height);
	}

	// Make sure video is playing, else there is nothing to grab
	if (!isVideoPlaying())
	{
		return -1;
	}


	// If the device is not open, attempt to open it
	if (_amlogicCaptureDev == -1)
	{
		_amlogicCaptureDev = open("/dev/amvideocap0", O_RDONLY, 0);

		// If the device is still not open, there is something wrong
		if (_amlogicCaptureDev == -1)
		{
			std::cerr << "AMLOGICGRABBER ERROR:[" << __PRETTY_FUNCTION__ << "] Failed to open the AMLOGIC device (" << errno << "): " << strerror(errno) << std::endl;
			return -1;
		}
	}


	if (ioctl(_amlogicCaptureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,  _width)  == -1 ||
		ioctl(_amlogicCaptureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, _height) == -1)
	{
		// Failed to configure frame width
		std::cerr << "AMLOGICGRABBER ERROR: [" << __PRETTY_FUNCTION__ << "] Failed to configure capture size (" << errno << "): " << strerror(errno) << std::endl;
		close(_amlogicCaptureDev);
		_amlogicCaptureDev = -1;
		return -1;
	}

	// Read the snapshot into the memory
	void * image_ptr = image.memptr();
	const ssize_t bytesToRead = _width * _height * sizeof(ColorBgr);

	const ssize_t bytesRead   = pread(_amlogicCaptureDev, image_ptr, bytesToRead, 0);
	if (bytesRead == -1)
	{
		std::cerr << "AMLOGICGRABBER ERROR: [" << __PRETTY_FUNCTION__ << "] Read of device failed (erno=" << errno << "): " << strerror(errno) << std::endl;
		close(_amlogicCaptureDev);
		_amlogicCaptureDev = -1;
		return -1;
	}
	else if (bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		std::cerr << "AMLOGICGRABBER ERROR: [" << __PRETTY_FUNCTION__ << "] Capture failed to grab entire image [bytesToRead(" << bytesToRead << ") != bytesRead(" << bytesRead << ")]" << std::endl;
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
	return 0;
}
