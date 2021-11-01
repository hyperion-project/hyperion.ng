#pragma once
#ifndef __APPLE__

/*
 * this is a mock up for compiling and testing osx wrapper on no osx platform.
 * this will show a test image and rotate the colors.
 *
 * see https://github.com/phracker/MacOSX-SDKs/blob/master/MacOSX10.8.sdk/System/Library/Frameworks/CoreGraphics.framework/Versions/A/Headers
 * 
 */

#include <utils/Image.h>
#include <utils/ColorRgb.h>

enum _CGError {
	kCGErrorSuccess = 0,
	kCGErrorFailure = 1000,
	kCGErrorIllegalArgument = 1001,
	kCGErrorInvalidConnection = 1002,
	kCGErrorInvalidContext = 1003,
	kCGErrorCannotComplete = 1004,
	kCGErrorNotImplemented = 1006,
	kCGErrorRangeCheck = 1007,
	kCGErrorTypeCheck = 1008,
	kCGErrorInvalidOperation = 1010,
	kCGErrorNoneAvailable = 1011,

	/* Obsolete errors. */
	kCGErrorNameTooLong = 1005,
	kCGErrorNoCurrentPoint = 1009,
	kCGErrorApplicationRequiresNewerSystem = 1015,
	kCGErrorApplicationNotPermittedToExecute = 1016,
	kCGErrorApplicationIncorrectExecutableFormatFound = 1023,
	kCGErrorApplicationIsLaunching = 1024,
	kCGErrorApplicationAlreadyRunning = 1025,
	kCGErrorApplicationCanOnlyBeRunInOneSessionAtATime = 1026,
	kCGErrorClassicApplicationsMustBeLaunchedByClassic = 1027,
	kCGErrorForkFailed = 1028,
	kCGErrorRetryRegistration = 1029,
	kCGErrorFirst = 1000,
	kCGErrorLast = 1029
};
typedef int32_t CGError;
typedef double CGFloat;

struct CGSize {
	CGFloat width;
	CGFloat height;
};
typedef struct CGSize CGSize;

struct CGPoint {
	float x;
	float y;
};
typedef struct CGPoint CGPoint;

struct CGRect {
	CGPoint origin;
	CGSize size;
};
typedef struct CGRect CGRect;

typedef CGError CGDisplayErr;
typedef uint32_t CGDirectDisplayID;
typedef uint32_t CGDisplayCount;;
typedef struct CGDisplayMode *CGDisplayModeRef;

typedef Image<ColorRgb> CGImage;
typedef CGImage* CGImageRef;
typedef unsigned char CFData;
typedef CFData* CFDataRef;

const int kCGDirectMainDisplay = 0;

CGError CGGetActiveDisplayList(uint32_t maxDisplays, CGDirectDisplayID *activeDisplays, uint32_t *displayCount);
CGDisplayModeRef CGDisplayCopyDisplayMode(CGDirectDisplayID display);
CGRect           CGDisplayBounds(CGDirectDisplayID display);
void             CGDisplayModeRelease(CGDisplayModeRef mode);

CGImageRef       CGDisplayCreateImage(CGDirectDisplayID display);
void             CGImageRelease(CGImageRef image);
CGImageRef       CGImageGetDataProvider(CGImageRef image);
CFDataRef        CGDataProviderCopyData(CGImageRef image);
unsigned char*   CFDataGetBytePtr(CFDataRef imgData);
unsigned         CGImageGetWidth(CGImageRef image);
unsigned         CGImageGetHeight(CGImageRef image);
unsigned         CGImageGetBitsPerPixel(CGImageRef image);
unsigned         CGImageGetBytesPerRow(CGImageRef image);
void             CFRelease(CFDataRef imgData);


#endif
