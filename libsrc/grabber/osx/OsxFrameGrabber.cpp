// STL includes
#include <cassert>
#include <iostream>

// Local includes
#include <grabber/OsxFrameGrabber.h>

OsxFrameGrabber::OsxFrameGrabber(const unsigned display, const unsigned width, const unsigned height)
	: Grabber("OSXGRABBER", width, height)
	, _screenIndex(display)
{
	CGImageRef image;
	CGDisplayCount displayCount;
	CGDirectDisplayID displays[8];

	// get list of displays
	CGGetActiveDisplayList(8, displays, &displayCount);
	if (_screenIndex + 1 > displayCount)
	{
		Error(_log, "Display with index %d is not available. Using main display", _screenIndex);
		_display = kCGDirectMainDisplay;
	}
	else
	{
		_display = displays[_screenIndex];
	}
		
	image = CGDisplayCreateImage(_display);
	assert(image != NULL);

	Info(_log, "Display opened with resolution: %dx%d@%dbit", CGImageGetWidth(image), CGImageGetHeight(image), CGImageGetBitsPerPixel(image));

	CGImageRelease(image);
}

OsxFrameGrabber::~OsxFrameGrabber()
{
}

int OsxFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	CGImageRef dispImage;
	CFDataRef imgData;
	unsigned char * pImgData;	
	unsigned dspWidth, dspHeight;
	
	dispImage = CGDisplayCreateImage(_display);
	
	// display lost, use main
	if (dispImage == NULL && _display)
	{
		dispImage = CGDisplayCreateImage(kCGDirectMainDisplay);
		// no displays connected, return
		if (dispImage == NULL)
		{
			Error(_log, "No display connected...");
			return -1;
		}
	}
	imgData   = CGDataProviderCopyData(CGImageGetDataProvider(dispImage));
	pImgData  = (unsigned char*) CFDataGetBytePtr(imgData);
	dspWidth  = CGImageGetWidth(dispImage);
	dspHeight = CGImageGetHeight(dispImage);
	
	_imageResampler.setHorizontalPixelDecimation(dspWidth/_width);
	_imageResampler.setVerticalPixelDecimation(dspHeight/_height);
	_imageResampler.processImage( pImgData,
								dspWidth,
								dspHeight,
								CGImageGetBytesPerRow(dispImage),
								PIXELFORMAT_BGR32,
								image);
	
	CFRelease(imgData);
	CGImageRelease(dispImage);

	return 0;
}
