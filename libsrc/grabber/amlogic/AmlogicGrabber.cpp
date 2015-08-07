
// STL includes
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
#include "AmlogicGrabber.h"

// Flags copied from 'linux/amlogic/amports/amvideocap.h' at https://github.com/codesnake/linux-amlogic/
#define AMVIDEOCAP_IOC_MAGIC 'V'
#define AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH      		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x02, int)
#define AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT     		_IOW(AMVIDEOCAP_IOC_MAGIC, 0x03, int)

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height) :
	_width(width),
	_height(height),
	_amlogicCaptureDev(-1)
{
	_amlogicCaptureDev = open("/dev/amvideocap0", O_RDONLY, 0);
	if (_amlogicCaptureDev == -1)
	{
		std::cerr << "[" << __PRETTY_FUNCTION__ << "] Failed to open the AMLOGIC device (" << errno << ")" << std::endl;
		return;
	}
	
	if (ioctl(_amlogicCaptureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH,  _width)  == -1 || 
	    ioctl(_amlogicCaptureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, _height) == -1)
	{
		// Failed to configure frame width
		std::cerr << "[" << __PRETTY_FUNCTION__ << "] Failed to configure capture size (" << errno << ")" << std::endl;
	}
}

AmlogicGrabber::~AmlogicGrabber()
{
	if (_amlogicCaptureDev != -1)
	{
		if (close(_amlogicCaptureDev) == -1)
		{
			std::cerr << "[" << __PRETTY_FUNCTION__ << "] Failed to close AMLOGIC device (" << errno << ")" << std::endl;
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

void AmlogicGrabber::grabFrame(Image<ColorRgb> & image)
{
	// resize the given image if needed
	if (image.width() != unsigned(_rectangle.width) || image.height() != unsigned(_rectangle.height))
	{
		image.resize(_rectangle.width, _rectangle.height);
	}
	
	// Read the snapshot into the memory
	void * image_ptr = image.memptr();
	const size_t bytesToRead = _width * _height * sizeof(ColorRgb);
	const size_t bytesRead   = pread(amlogicCaptureDev, image_ptr, bytesToRead, 0)
	if (bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		std::cerr << "[" << __PRETTY_FUNCTION__ << "] Capture failed to grab entire image [bytesToRead(" << bytesToRead << ") != bytesRead(" << bytesRead << ")]" << std::endl;
	}
}
