#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

// STL includes
#include <iostream>

// Local includes
#include "FramebufferFrameGrabber.h"

FramebufferFrameGrabber::FramebufferFrameGrabber(const std::string & device, const unsigned width, const unsigned height) :
	_fbfd(0),
	_fbp(0),
	_fbDevice(device),
	_width(width),
	_height(height),
	_imgResampler(new ImageResampler())
{
	int result;
	struct fb_var_screeninfo vinfo;

	// Check if the framebuffer device can be opened and display the current resolution
	_fbfd = open(_fbDevice.c_str(), O_RDONLY);
	if (_fbfd == 0)
	{
		std::cerr << "Error openning " << _fbDevice << std::endl;
	}
	else 
	{
		// get variable screen information
		result = ioctl (_fbfd, FBIOGET_VSCREENINFO, &vinfo);
		if (result != 0)
		{
			std::cerr << "Could not get screen information" << std::endl;
		}
		else
		{
			std::cout << "Framebuffer opened with resolution: " << vinfo.xres << "x" << vinfo.yres << "@" << vinfo.bits_per_pixel << "bit" << std::endl;			
		}
		close(_fbfd);
	}
}

FramebufferFrameGrabber::~FramebufferFrameGrabber()
{
	delete _imgResampler;
}

void FramebufferFrameGrabber::setVideoMode(const VideoMode videoMode)
{
	_imgResampler->set3D(videoMode);
}

void FramebufferFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	struct fb_var_screeninfo vinfo;
	unsigned capSize, bytesPerPixel;
	PixelFormat pixelFormat;

	/* Open the framebuffer device */
	_fbfd = open(_fbDevice.c_str(), O_RDONLY);

	/* get variable screen information */
	ioctl (_fbfd, FBIOGET_VSCREENINFO, &vinfo);

	bytesPerPixel = vinfo.bits_per_pixel / 8;
	capSize = vinfo.xres * vinfo.yres * bytesPerPixel;
	
	if (vinfo.bits_per_pixel == 16)
	{
		pixelFormat = PIXELFORMAT_BGR16;
	}
	else if (vinfo.bits_per_pixel == 24)
	{
		pixelFormat = PIXELFORMAT_BGR24;
	}	
	else if (vinfo.bits_per_pixel == 32)
	{
		pixelFormat = PIXELFORMAT_BGR32;
	}
	else
	{
		std::cerr << "Unknown pixel format: " << vinfo.bits_per_pixel << " bits per pixel" << std::endl;
		close(_fbfd);
		return;
	}
			
	/* map the device to memory */
	_fbp = (unsigned char*)mmap(0, capSize, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, _fbfd, 0);	

	_imgResampler->setHorizontalPixelDecimation(vinfo.xres/_width);
	_imgResampler->setVerticalPixelDecimation(vinfo.yres/_height);
	_imgResampler->processImage(_fbp,
								vinfo.xres,
								vinfo.yres,
								vinfo.xres * bytesPerPixel,
								pixelFormat,
								image);
	
	munmap(_fbp, capSize);
	close(_fbfd);
}
