// STL includes
#include <cassert>
#include <iostream>
#include <mutex>

// Header
#include <grabber/osx/OsxFrameGrabber.h>

// ScreenCaptureKit
#if defined(SDK_15_AVAILABLE)
#include <ScreenCaptureKit/ScreenCaptureKit.h>
#endif

//Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

#if defined(SDK_15_AVAILABLE)
	static CGImageRef capture15(CGDirectDisplayID id, CGRect diIntersectDisplayLocal)
	{
		dispatch_semaphore_t semaphore = dispatch_semaphore_create(0);
		__block CGImageRef image1 = nil;
		[SCShareableContent getShareableContentWithCompletionHandler:^(SCShareableContent* content, NSError* error)
		{
			@autoreleasepool
			{
				if (error || !content)
				{
					dispatch_semaphore_signal(semaphore);
					return;
				}

				SCDisplay* target = nil;
				for (SCDisplay *display in content.displays)
				{
					if (display.displayID == id)
					{
						target = display;
						break;
					}
				}
				if (!target)
				{
					dispatch_semaphore_signal(semaphore);
					return;
				}

				SCContentFilter* filter = [[SCContentFilter alloc] initWithDisplay:target excludingWindows:@[]];
				SCStreamConfiguration* config = [[SCStreamConfiguration alloc] init];
				config.queueDepth = 5;
				config.sourceRect = diIntersectDisplayLocal;
				config.scalesToFit = false;
				config.captureResolution = SCCaptureResolutionBest;

				CGDisplayModeRef modeRef = CGDisplayCopyDisplayMode(id);
				double sysScale = CGDisplayModeGetPixelWidth(modeRef) / CGDisplayModeGetWidth(modeRef);
				config.width = diIntersectDisplayLocal.size.width * sysScale;
				config.height = diIntersectDisplayLocal.size.height * sysScale;

				[SCScreenshotManager captureImageWithFilter:filter
					configuration:config
					completionHandler:^(CGImageRef img, NSError* error)
					{
						if (!error)
						{
							image1 = CGImageCreateCopyWithColorSpace(img, CGColorSpaceCreateDeviceRGB());
						}
						dispatch_semaphore_signal(semaphore);
				}];
			}
		}];

		dispatch_semaphore_wait(semaphore, DISPATCH_TIME_FOREVER);
		dispatch_release(semaphore);
		return image1;
	}
#endif

OsxFrameGrabber::OsxFrameGrabber(int display)
	: Grabber("GRABBER-OSX")
	, _screenIndex(display)
{
	_isEnabled = false;
	_useImageResampler = true;
}

OsxFrameGrabber::~OsxFrameGrabber()
{
}

bool OsxFrameGrabber::setupDisplay()
{
	bool rc (false);

#if defined(SDK_15_AVAILABLE)
	if (!CGPreflightScreenCaptureAccess())
	{
		if(!CGRequestScreenCaptureAccess())
			Error(_log, "Screen capture permission required to start the grabber");
			return false;
	}
#endif

	rc = setDisplayIndex(_screenIndex);

	return rc;
}

int OsxFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (_isDeviceInError)
    {
        Error(_log, "Cannot grab frame, device is in error state");
        return -1;
    }

    if (!_isEnabled)
    {
        return -1;
    }

	if (image.isNull())
	{
		// cannot grab into a null image
		return -1;
	}

	CGImageRef dispImage;

	#if defined(SDK_15_AVAILABLE)
		dispImage = capture15(_display, CGDisplayBounds(_display));
	#else
		dispImage = CGDisplayCreateImageForRect(_display, CGDisplayBounds(_display));
	#endif

	// display lost, use main
	if (dispImage == nullptr && _display != 0)
	{
		#if defined(SDK_15_AVAILABLE)
			dispImage = capture15(kCGDirectMainDisplay, CGDisplayBounds(kCGDirectMainDisplay));
		#else
			dispImage = CGDisplayCreateImageForRect(kCGDirectMainDisplay, CGDisplayBounds(kCGDirectMainDisplay));
		#endif
	}

	// no displays connected, return
	if (dispImage == nullptr)
	{
		Error(_log, "No display connected...");
		return -1;
	}

	CFDataRef imgData = CGDataProviderCopyData(CGImageGetDataProvider(dispImage));
	if (imgData != nullptr)
	{
		_imageResampler.processImage((uint8_t *)CFDataGetBytePtr(imgData), static_cast<int>(CGImageGetWidth(dispImage)), static_cast<int>(CGImageGetHeight(dispImage)), static_cast<int>(CGImageGetBytesPerRow(dispImage)), PixelFormat::BGR32, image);
		CFRelease(imgData);
	}

	CGImageRelease(dispImage);


	return 0;
}

bool OsxFrameGrabber::setDisplayIndex(int index)
{
	bool rc (true);
	if(_screenIndex != index || !_isEnabled)
	{
		_screenIndex = index;

		// get list of displays
		CGDisplayCount dspyCnt = 0 ;
		CGDisplayErr err;
		err = CGGetActiveDisplayList(0, nullptr, &dspyCnt);
		if (err == kCGErrorSuccess && dspyCnt > 0)
		{
			CGDirectDisplayID *activeDspys = new CGDirectDisplayID [dspyCnt] ;
			err = CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt) ;
			if (err == kCGErrorSuccess)
			{
				CGImageRef image;

				if (_screenIndex + 1 > static_cast<int>(dspyCnt))
				{
					Error(_log, "Display with index %d is not available.", _screenIndex);
					rc = false;
				}
				else
				{
					_display = activeDspys[_screenIndex];

					#if defined(SDK_15_AVAILABLE)
						image = capture15(_display, CGDisplayBounds(_display));
					#else
						image = CGDisplayCreateImageForRect(_display, CGDisplayBounds(_display));
					#endif

					if(image == nullptr)
					{
						setEnabled(false);
						Error(_log, "Failed to open main display, disable capture interface");
						rc = false;
					}
					else
					{
						setEnabled(true);
						rc = true;
						Info(_log, "Display [%u] opened with resolution: %ux%u@%ubit", _display, CGImageGetWidth(image), CGImageGetHeight(image), CGImageGetBitsPerPixel(image));
					}
					CGImageRelease(image);
				}
			}
		delete[] activeDspys;
		}
		else
		{
			rc=false;
		}
	}
	return rc;
}

QJsonObject OsxFrameGrabber::discover(const QJsonObject& params)
{
	QJsonObject inputsDiscovered;

	// get list of displays
	CGDisplayCount dspyCnt = 0 ;
	CGDisplayErr err;
	err = CGGetActiveDisplayList(0, nullptr, &dspyCnt);
	if (err == kCGErrorSuccess && dspyCnt > 0)
	{
		CGDirectDisplayID *activeDspys = new CGDirectDisplayID [dspyCnt] ;
		err = CGGetActiveDisplayList(dspyCnt, activeDspys, &dspyCnt) ;
		if (err == kCGErrorSuccess)
		{
			inputsDiscovered["device"] = "osx";
			inputsDiscovered["device_name"] = "OSX";
			inputsDiscovered["type"] = "screen";

			QJsonArray video_inputs;
			QJsonArray fps = { 1, 5, 10, 15, 20, 25, 30, 40, 50, 60 };

			for (int i = 0; i < static_cast<int>(dspyCnt); ++i)
			{
				QJsonObject in;

				CGDirectDisplayID did = activeDspys[i];

				QString displayName;
				displayName = QString("Display:%1").arg(did);

				in["name"] = displayName;
				in["inputIdx"] = i;

				QJsonArray formats;
				QJsonObject format;

				QJsonArray resolutionArray;

				QJsonObject resolution;


				CGDisplayModeRef dispMode = CGDisplayCopyDisplayMode(did);
				CGRect rect = CGDisplayBounds(did);
				resolution["width"] = static_cast<int>(rect.size.width);
				resolution["height"] = static_cast<int>(rect.size.height);
				CGDisplayModeRelease(dispMode);

				resolution["fps"] = fps;

				resolutionArray.append(resolution);

				format["resolutions"] = resolutionArray;
				formats.append(format);

				in["formats"] = formats;
				video_inputs.append(in);
			}
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = _fps;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}
		delete[] activeDspys;
	}

	if (inputsDiscovered.isEmpty())
	{
		qCDebug(grabber_screen_properties) << "No displays found to capture from!";
	}

	return inputsDiscovered;
}
