// STL includes
#include <cassert>
#include <iostream>

// Local includes
#include <grabber/OsxFrameGrabber.h>

//Qt
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// Constants
namespace {
const bool verbose = false;
} //End of constants

OsxFrameGrabber::OsxFrameGrabber(int display)
	: Grabber("OSXGRABBER")
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

	rc = setDisplayIndex(_screenIndex);

	return rc;
}

int OsxFrameGrabber::grabFrame(Image<ColorRgb> & image)
{
	int rc = 0;
	if (_isEnabled && !_isDeviceInError)
	{

		CGImageRef dispImage;
		CFDataRef imgData;
		unsigned char * pImgData;
		unsigned dspWidth;
		unsigned dspHeight;

		dispImage = CGDisplayCreateImage(_display);

		// display lost, use main
		if (dispImage == nullptr && _display != 0)
		{
			dispImage = CGDisplayCreateImage(kCGDirectMainDisplay);
			// no displays connected, return
			if (dispImage == nullptr)
			{
				Error(_log, "No display connected...");
				return -1;
			}
		}
		imgData   = CGDataProviderCopyData(CGImageGetDataProvider(dispImage));
		pImgData  = (unsigned char*) CFDataGetBytePtr(imgData);
		dspWidth  = CGImageGetWidth(dispImage);
		dspHeight = CGImageGetHeight(dispImage);

		_imageResampler.processImage( pImgData,
									  static_cast<int>(dspWidth),
									  static_cast<int>(dspHeight),
									  static_cast<int>(CGImageGetBytesPerRow(dispImage)),
									  PixelFormat::BGR32,
									  image);

		CFRelease(imgData);
		CGImageRelease(dispImage);

	}
	return rc;
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

					image = CGDisplayCreateImage(_display);
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
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

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
		delete [] activeDspys;
	}

	if (inputsDiscovered.isEmpty())
	{
		DebugIf(verbose, _log, "No displays found to capture from!");
	}
	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;

}
