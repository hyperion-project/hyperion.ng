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
#include <grabber/FramebufferFrameGrabber.h>

FramebufferFrameGrabber::FramebufferFrameGrabber(const QString & device, const unsigned width, const unsigned height)
	: Grabber("FRAMEBUFFERGRABBER", width, height)
	, _fbfd(0)
	, _fbp(0)
	, _fbDevice()
{
	setDevicePath(device);
}

FramebufferFrameGrabber::~FramebufferFrameGrabber()
{
}

int FramebufferFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	struct fb_var_screeninfo vinfo;
	unsigned capSize, bytesPerPixel;
	PixelFormat pixelFormat;

	/* Open the framebuffer device */
	_fbfd = open(QSTRING_CSTR(_fbDevice), O_RDONLY);

	/* get variable screen information */
	ioctl (_fbfd, FBIOGET_VSCREENINFO, &vinfo);

	bytesPerPixel = vinfo.bits_per_pixel / 8;
	capSize = vinfo.xres * vinfo.yres * bytesPerPixel;

	switch (vinfo.bits_per_pixel)
	{
		case 16: pixelFormat = PIXELFORMAT_BGR16; break;
		case 24: pixelFormat = PIXELFORMAT_BGR24; break;
#ifdef ENABLE_AMLOGIC
		case 32: pixelFormat = PIXELFORMAT_RGB32; break;
#else
		case 32: pixelFormat = PIXELFORMAT_BGR32; break;
#endif
		default:
			Error(_log, "Unknown pixel format: %d bits per pixel", vinfo.bits_per_pixel);
			close(_fbfd);
			return -1;
	}

	/* map the device to memory */
	_fbp = (unsigned char*)mmap(0, capSize, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, _fbfd, 0);

	_imageResampler.setHorizontalPixelDecimation(vinfo.xres/_width);
	_imageResampler.setVerticalPixelDecimation(vinfo.yres/_height);
	_imageResampler.processImage(_fbp,
								vinfo.xres,
								vinfo.yres,
								vinfo.xres * bytesPerPixel,
								pixelFormat,
								image);

	munmap(_fbp, capSize);
	close(_fbfd);

	return 0;
}

void FramebufferFrameGrabber::setDevicePath(const QString& path)
{
	if(_fbDevice != path)
	{
		_fbDevice = path;
		int result;
		struct fb_var_screeninfo vinfo;

		// Check if the framebuffer device can be opened and display the current resolution
		_fbfd = open(QSTRING_CSTR(_fbDevice), O_RDONLY);
		if (_fbfd == 0)
		{
			Error(_log, "Error openning %s", QSTRING_CSTR(_fbDevice));
		}
		else
		{
			// get variable screen information
			result = ioctl (_fbfd, FBIOGET_VSCREENINFO, &vinfo);
			if (result != 0)
			{
				Error(_log, "Could not get screen information");
			}
			else
			{
				Info(_log, "Display opened with resolution: %dx%d@%dbit", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
			}
			close(_fbfd);
		}

	}
}
