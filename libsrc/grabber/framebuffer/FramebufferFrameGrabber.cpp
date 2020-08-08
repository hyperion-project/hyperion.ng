#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <cstring>

// STL includes
#include <iostream>

// Local includes
#include <grabber/FramebufferFrameGrabber.h>

FramebufferFrameGrabber::FramebufferFrameGrabber(const QString & device, unsigned width, unsigned height)
	: Grabber("FRAMEBUFFERGRABBER", width, height)
	, _fbDevice()
{
	setDevicePath(device);
}

int FramebufferFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	struct fb_var_screeninfo vinfo;
	unsigned capSize, bytesPerPixel;
	PixelFormat pixelFormat;

	/* Open the framebuffer device */
	int fbfd = open(QSTRING_CSTR(_fbDevice), O_RDONLY);
	if (fbfd == -1)
	{
		Error(_log, "Error opening %s, %s : ", QSTRING_CSTR(_fbDevice), std::strerror(errno));
		return -1;
	}

	/* get variable screen information */
	ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);

	bytesPerPixel = vinfo.bits_per_pixel / 8;
	capSize = vinfo.xres * vinfo.yres * bytesPerPixel;

	switch (vinfo.bits_per_pixel)
	{
		case 16: pixelFormat = PixelFormat::BGR16; break;
		case 24: pixelFormat = PixelFormat::BGR24; break;
#ifdef ENABLE_AMLOGIC
		case 32: pixelFormat = PixelFormat::PIXELFORMAT_RGB32; break;
#else
		case 32: pixelFormat = PixelFormat::BGR32; break;
#endif
		default:
			Error(_log, "Unknown pixel format: %d bits per pixel", vinfo.bits_per_pixel);
			close(fbfd);
			return -1;
	}

	/* map the device to memory */
	unsigned char * fbp = (unsigned char*)mmap(0, capSize, PROT_READ, MAP_PRIVATE | MAP_NORESERVE, fbfd, 0);
	if (fbp == MAP_FAILED) {
		Error(_log, "Error mapping %s, %s : ", QSTRING_CSTR(_fbDevice), std::strerror(errno));
		return -1;
	}

	_imageResampler.setHorizontalPixelDecimation(vinfo.xres/_width);
	_imageResampler.setVerticalPixelDecimation(vinfo.yres/_height);
	_imageResampler.processImage(fbp,
								vinfo.xres,
								vinfo.yres,
								vinfo.xres * bytesPerPixel,
								pixelFormat,
								image);

	munmap(fbp, capSize);
	close(fbfd);

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
		int fbfd = open(QSTRING_CSTR(_fbDevice), O_RDONLY);
		if (fbfd == -1)
		{
			Error(_log, "Error opening %s, %s : ", QSTRING_CSTR(_fbDevice), std::strerror(errno));
		}
		else
		{
			// get variable screen information
			result = ioctl (fbfd, FBIOGET_VSCREENINFO, &vinfo);
			if (result != 0)
			{
				Error(_log, "Could not get screen information, %s", std::strerror(errno));
			}
			else
			{
				Info(_log, "Display opened with resolution: %dx%d@%dbit", vinfo.xres, vinfo.yres, vinfo.bits_per_pixel);
			}
			close(fbfd);
		}
	}
}
