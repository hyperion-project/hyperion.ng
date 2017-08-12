#pragma once
#ifndef __APPLE__

/*
 * this is a mock up for compiling and testing osx wrapper on no osx platform.
 * this will show a test image and rotate the colors.
 * 
 */

#include <utils/Image.h>
#include <utils/ColorRgb.h>

typedef int CGDirectDisplayID;
typedef Image<ColorRgb> CGImage;
typedef CGImage* CGImageRef;
typedef unsigned char CFData;
typedef CFData* CFDataRef;
typedef unsigned CGDisplayCount;

const int kCGDirectMainDisplay = 0;

void           CGGetActiveDisplayList(int max, CGDirectDisplayID *displays, CGDisplayCount *displayCount);
CGImageRef     CGDisplayCreateImage(CGDirectDisplayID display);
void           CGImageRelease(CGImageRef image);
CGImageRef     CGImageGetDataProvider(CGImageRef image);
CFDataRef      CGDataProviderCopyData(CGImageRef image);
unsigned char* CFDataGetBytePtr(CFDataRef imgData);
unsigned       CGImageGetWidth(CGImageRef image);
unsigned       CGImageGetHeight(CGImageRef image);
unsigned       CGImageGetBitsPerPixel(CGImageRef image);
unsigned       CGImageGetBytesPerRow(CGImageRef image);
void           CFRelease(CFDataRef imgData);

#endif
