#pragma once

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <fcntl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include <utils/RgbImage.h>

class FbWriter
{
public:
	FbWriter()
	{
		initialise();
	}

	~FbWriter()
	{
		if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &orig_vinfo))
		{
			printf("Error re-setting variable information.\n");
		}

		close(fbfd);
	}

	int initialise()
	{
		// Open the file for reading and writing
		fbfd = open("/dev/fb0", O_RDWR);
		if (!fbfd)
		{
			printf("Error: cannot open framebuffer device.\n");
			return(-1);
		}
		printf("The framebuffer device was opened successfully.\n");

		// Get fixed screen information
		if (ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo))
		{
			printf("Error reading fixed information.\n");
		}
		// Get variable screen information
		if (ioctl(fbfd, FBIOGET_VSCREENINFO, &orig_vinfo))
		{
			printf("Error reading variable information.\n");
		}
		printf("Original %dx%d, %dbpp\n", orig_vinfo.xres, orig_vinfo.yres, orig_vinfo.bits_per_pixel );


		return 0;
	}

	void writeImage(const RgbImage& image)
	{
		std::cout << "Writing image [" << image.width() << "x" << image.height() << "]" << std::endl;

		// Make a copy of the original screen-info
		fb_var_screeninfo vinfo = orig_vinfo;
//		memcpy(&vinfo, &orig_vinfo, sizeof(fb_var_screeninfo));

		// Configure the frame-buffer for the new image
		vinfo.xres = image.width();
		vinfo.yres = image.height();
		vinfo.bits_per_pixel = 24;
		if (ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo))
		{
			printf("Error configuring frame-buffer");
		}

		ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
		std::cout << "New set resolution: " << vinfo.xres << "x" << vinfo.yres << std::endl;

		// map fb to user mem
		long screensize = vinfo.yres * finfo.line_length;//vinfo.yres * vinfo.bits_per_pixel / 8;
		char* fbp = (char*)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fbfd,  0);
		if (!fbp)
		{
			// Failed to create memory map
			std::cout << "Failed to create the memory map" << std::endl;
			return;
		}
		std::cout << "Screensize  : " << screensize << std::endl;
		std::cout << "Max fb-index: " << (image.width()-1)*3 + (image.height()-1)*finfo.line_length << std::endl;
		std::cout << "[" << vinfo.xres << "x" << vinfo.yres << "] == [" << image.width() << "x" << image.height() << "]" << std::endl;

		for (unsigned iY=0; iY<image.height(); ++iY)
		{
			memcpy(fbp + iY*finfo.line_length, &(image(0, iY)), image.width()*3);
//			for (unsigned iX=0; iX<image.width(); ++iX)
//			{
//				const unsigned pixOffset = iX*3 + iY*finfo.line_length;
//				fbp[pixOffset  ] = image(iX, iY).red;
//				fbp[pixOffset+1] = image(iX, iY).green;
//				fbp[pixOffset+2] = image(iX, iY).blue;
//			}
		}
		std::cout << "FINISHED COPYING IMAGE TO FRAMEBUFFER" << std::endl;
		// cleanup
		munmap(fbp, screensize);
	}

	// The identifier of the FrameBuffer File-Device
	int fbfd;
	// The 'Fixed' screen information
	fb_fix_screeninfo finfo;
	// The original 'Variable' screen information
	fb_var_screeninfo orig_vinfo;
};
