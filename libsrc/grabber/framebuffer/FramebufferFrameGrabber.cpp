#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <math.h>

// STL includes
#include <cassert>
#include <iostream>

// Local includes
#include "FramebufferFrameGrabber.h"

FramebufferFrameGrabber::FramebufferFrameGrabber(const std::string & device, const unsigned width, const unsigned height) :
	_fbfd(0),
	_fbp(0),
	_fbDevice(device),
	_width(width),
	_height(height),
	_xScale(1),
	_yScale(1)
{
	int result;
	struct fb_var_screeninfo vinfo;

	// Check if the framebuffer device can be opened and display the current resolution
	_fbfd = open(_fbDevice.c_str(), O_RDONLY);
	assert(_fbfd > 0);

	// get variable screen information
	result = ioctl (_fbfd, FBIOGET_VSCREENINFO, &vinfo);
	assert(result == 0);

	std::cout << "Framebuffer opened with resolution: " << vinfo.xres << "x" << vinfo.yres << "@" << vinfo.bits_per_pixel << "bit" << std::endl;

	close(_fbfd);
}

FramebufferFrameGrabber::~FramebufferFrameGrabber()
{
}

void FramebufferFrameGrabber::setVideoMode(const VideoMode videoMode)
{
	switch (videoMode) {
	case VIDEO_3DSBS:
		_xScale = 2;
		_yScale = 1;
		break;
	case VIDEO_3DTAB:
		_xScale = 1;
		_yScale = 2;
		break;
	case VIDEO_2D:
	default:
		_xScale = 1;
		_yScale = 1;
		break;
	}
}

void FramebufferFrameGrabber::grabFrame(Image<ColorRgba> & image)
{
	struct fb_var_screeninfo vinfo;
	unsigned capSize, px, py, index, bytesPerPixel;
	float x_scale, y_scale;

	/* resize the given image if needed */
	if (image.width() != _width || image.height() != _height)
	{
		image.resize(_width, _height);
	}

	/* Open the framebuffer device */
	_fbfd = open(_fbDevice.c_str(), O_RDONLY);

	/* get variable screen information */
	ioctl (_fbfd, FBIOGET_VSCREENINFO, &vinfo);

	bytesPerPixel = vinfo.bits_per_pixel / 8;
	capSize = vinfo.xres * vinfo.yres * bytesPerPixel;
		
	/* map the device to memory */
	_fbp = (unsigned char*)mmap(0, capSize, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, _fbfd, 0);	

	/* nearest neighbor downscaling */
    x_scale = (vinfo.xres / float(_xScale)) / float(_width);
    y_scale = (vinfo.yres / float(_yScale)) / float(_height);

	ColorRgba *pPixel = image.memptr();
	for (unsigned i=0; i < _height; i++) {
		for (unsigned j=0; j < _width; j++) {
			px = floor(j * x_scale);
			py = floor(i * y_scale);
			index = (py * vinfo.xres + px) * bytesPerPixel;

			if (vinfo.bits_per_pixel == 16) {
				pPixel->blue = (_fbp[index] & 0x1f) << 3;
				pPixel->green = (((_fbp[index + 1] & 0x7) << 3) | (_fbp[index] & 0xE0) >> 5) << 2;
				pPixel->red = (_fbp[index + 1] & 0xF8);
				pPixel->alpha = 255;
			} else if(vinfo.bits_per_pixel == 24) {
				pPixel->blue = _fbp[index];
				pPixel->green = _fbp[index + 1];
				pPixel->red = _fbp[index + 2];
				pPixel->alpha = 255;
			} else if(vinfo.bits_per_pixel == 32) {
				pPixel->blue = _fbp[index];
				pPixel->green = _fbp[index + 1];
				pPixel->red = _fbp[index + 2];
				pPixel->alpha = _fbp[index + 3];
			}
			
			pPixel++;
		}
	}
	
	munmap(_fbp, capSize);
	close(_fbfd);
}
