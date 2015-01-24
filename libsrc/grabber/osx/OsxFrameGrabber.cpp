// STL includes
#include <cassert>
#include <iostream>

// Local includes
#include "OsxFrameGrabber.h"

OsxFrameGrabber::OsxFrameGrabber(const unsigned display, const unsigned width, const unsigned height) :
	_screenIndex(display),
	_width(width),
	_height(height),
	_imgResampler(new ImageResampler())
{
	CGImageRef image;
	CGDisplayCount displayCount;
	CGDirectDisplayID displays[8];

	// get list of displays
	CGGetActiveDisplayList(8, displays, &displayCount);
	if (_screenIndex + 1 > displayCount)
	{
		std::cerr << "OSX display with index " << _screenIndex << " is not available. Using main display" << std::endl;
		_display = kCGDirectMainDisplay;
	} else {
		_display = displays[_screenIndex];
	}
		
	image = CGDisplayCreateImage(_display);
	assert(image != NULL);

	std::cout << "OSX display opened with resolution: " << CGImageGetWidth(image) << "x" << CGImageGetHeight(image) << "@" << CGImageGetBitsPerPixel(image) << "bit" << std::endl;

	CGImageRelease(image);
}

OsxFrameGrabber::~OsxFrameGrabber()
{
	delete _imgResampler;
}

void OsxFrameGrabber::setVideoMode(const VideoMode videoMode)
{
	_imgResampler->set3D(videoMode);
}

void OsxFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	CGImageRef dispImage;
	CFDataRef imgData;
	unsigned char * pImgData;	
	unsigned dspWidth, dspHeight;
	
	dispImage = CGDisplayCreateImage(_display);
	
	// dsiplay lost, use main
	if (dispImage == NULL && _display)
	{
		dispImage = CGDisplayCreateImage(kCGDirectMainDisplay);
		// no displays connected, return
		if (dispImage == NULL)
		{
			std::cerr << "OSX no display connected..." << std::endl;
			return;
		}
	}
	imgData = CGDataProviderCopyData(CGImageGetDataProvider(dispImage));
	pImgData = (unsigned char*) CFDataGetBytePtr(imgData);
	dspWidth = CGImageGetWidth(dispImage);
	dspHeight = CGImageGetHeight(dispImage);
	
	_imgResampler->setHorizontalPixelDecimation(dspWidth/_width);
	_imgResampler->setVerticalPixelDecimation(dspHeight/_height);
	_imgResampler->processImage( pImgData,
								dspWidth,
								dspHeight,
								CGImageGetBytesPerRow(dispImage),
								PIXELFORMAT_BGR32,
								image);
	
	CFRelease(imgData);
	CGImageRelease(dispImage);
}
