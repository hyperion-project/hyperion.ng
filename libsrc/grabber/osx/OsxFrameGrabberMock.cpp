#ifndef __APPLE__
#include <grabber/OsxFrameGrabberMock.h>

unsigned __osx_frame_counter = 0;
const int __screenWidth  = 800;
const int __screenHeight = 600;

CGError CGGetActiveDisplayList(uint32_t maxDisplays, CGDirectDisplayID *activeDisplays, uint32_t *displayCount)
{
	if (maxDisplays == 0 || activeDisplays == nullptr)
	{
		*displayCount = 2;
	}
	else
	{
		displayCount = &maxDisplays;
		if (activeDisplays != nullptr)
		{
			for (CGDirectDisplayID i = 0; i < maxDisplays; ++i)
			{
				activeDisplays[i] = i;
			}
		}
		else
		{
			return kCGErrorFailure;
		}
	}
	return kCGErrorSuccess;
}

CGImageRef CGDisplayCreateImage(CGDirectDisplayID display)
{
	CGImageRef image = new CGImage(__screenWidth / (display+1), __screenHeight / (display+1));

	return image;
}

void CGImageRelease(CGImageRef image)
{
	delete image;
}

CGImageRef CGImageGetDataProvider(CGImageRef image)
{
	__osx_frame_counter++;
	if (__osx_frame_counter > 100)
	{
		__osx_frame_counter = 0;
	}

	ColorRgb color[4] = {ColorRgb::RED, ColorRgb::BLUE, ColorRgb::GREEN, ColorRgb::WHITE};
	if (__osx_frame_counter < 25)
	{
		color[0] = ColorRgb::WHITE;
		color[1] = ColorRgb::RED;
		color[2] = ColorRgb::BLUE;
		color[3] = ColorRgb::GREEN;
	}
	else if(__osx_frame_counter < 50)
	{
		color[1] = ColorRgb::WHITE;
		color[2] = ColorRgb::RED;
		color[3] = ColorRgb::BLUE;
		color[0] = ColorRgb::GREEN;
	}
	else if(__osx_frame_counter < 75)
	{
		color[2] = ColorRgb::WHITE;
		color[3] = ColorRgb::RED;
		color[0] = ColorRgb::BLUE;
		color[1] = ColorRgb::GREEN;
	}
	unsigned w = image->width();
	unsigned h = image->height();

	for (unsigned y=0; y<h; y++)
	{
		for (unsigned x=0; x<w; x++)
		{
			unsigned id = 0;
			if (x  < w/2 && y  < h/2) id = 1;
			if (x  < w/2 && y >= h/2) id = 2;
			if (x >= w/2 && y  < h/2) id = 3;

			image->memptr()[y*w + x] = color[id];
		}
	}

	return image;
}

CFDataRef CGDataProviderCopyData(CGImageRef image)
{
	const unsigned indexMax = image->width() * image->height() * CGImageGetBitsPerPixel(image);
	CFDataRef data = new CFData[indexMax];
	int lineLength = CGImageGetBytesPerRow(image);

	for (unsigned y=0; y<image->height(); y++)
	{
		for (unsigned x=0; x<image->width(); x++)
		{
			int index = lineLength * y + x * CGImageGetBitsPerPixel(image);

			data[index  ] = (*image)(x,y).blue;
			data[index+1] = (*image)(x,y).green;
			data[index+2] = (*image)(x,y).red;
			data[index+3] = 0;
		}
	}
	return data;
}

unsigned char* CFDataGetBytePtr(CFDataRef imgData)
{
	return imgData;
}

unsigned CGImageGetWidth(CGImageRef image)
{
	return image->width();
}

unsigned CGImageGetHeight(CGImageRef image)
{
	return image->height();
}

unsigned CGImageGetBytesPerRow(CGImageRef image)
{
	return image->width()*CGImageGetBitsPerPixel(image);
}

unsigned CGImageGetBitsPerPixel(CGImageRef)
{
	return 4;
}

void CFRelease(CFDataRef imgData)
{
	delete imgData;
}

CGDisplayModeRef CGDisplayCopyDisplayMode(CGDirectDisplayID display)
{
	return nullptr;
}
CGRect CGDisplayBounds(CGDirectDisplayID display)
{
	CGRect rect;
	rect.size.width  = __screenWidth / (display+1);
	rect.size.height = __screenHeight / (display+1);
	return rect;
}
void CGDisplayModeRelease(CGDisplayModeRef mode)
{
}

#endif
