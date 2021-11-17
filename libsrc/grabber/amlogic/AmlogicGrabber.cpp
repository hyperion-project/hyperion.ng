// STL includes
#include <algorithm>
#include <cassert>
#include <iostream>

// Linux includes
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

// qt
#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QSize>

// Local includes
#include <utils/Logger.h>
#include <grabber/AmlogicGrabber.h>
#include "Amvideocap.h"

// Constants
namespace {
const bool verbose = false;

const char DEFAULT_FB_DEVICE[] = "/dev/fb0";
const char DEFAULT_VIDEO_DEVICE[] = "/dev/amvideo";
const char DEFAULT_CAPTURE_DEVICE[] = "/dev/amvideocap0";
const int  AMVIDEOCAP_WAIT_MAX_MS = 40;
const int  AMVIDEOCAP_DEFAULT_RATE_HZ = 25;

} //End of constants

AmlogicGrabber::AmlogicGrabber()
	: Grabber("AMLOGICGRABBER") // Minimum required width or height is 160
	  , _captureDev(-1)
	  , _videoDev(-1)
	  , _lastError(0)
	  , _fbGrabber(DEFAULT_FB_DEVICE)
	  , _grabbingModeNotification(0)
{
	_image_ptr = _image_bgr.memptr();
	_useImageResampler = true;
}

AmlogicGrabber::~AmlogicGrabber()
{
	closeDevice(_captureDev);
	closeDevice(_videoDev);
}

bool AmlogicGrabber::setupScreen()
{
	bool rc (false);

	QSize screenSize = _fbGrabber.getScreenSize(DEFAULT_FB_DEVICE);
	if ( !screenSize.isEmpty() )
	{
		if (setWidthHeight(screenSize.width(), screenSize.height()))
		{
			rc = _fbGrabber.setupScreen();
		}
	}
	return rc;
}

bool AmlogicGrabber::openDevice(int &fd, const char* dev)
{
	bool rc = true;
	if (fd<0)
	{
		fd = ::open(dev, O_RDWR);
		if ( fd < 0)
		{
			rc = false;
		}
	}
	return rc;
}

void AmlogicGrabber::closeDevice(int &fd)
{
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
}

bool AmlogicGrabber::isVideoPlaying()
{
	bool rc = false;
	if(QFile::exists(DEFAULT_VIDEO_DEVICE))
	{
		int videoDisabled = 1;
		if (!openDevice(_videoDev, DEFAULT_VIDEO_DEVICE))
		{
			Error(_log, "Failed to open video device(%s): %d - %s", DEFAULT_VIDEO_DEVICE, errno, strerror(errno));
		}
		else
		{
			// Check the video disabled flag
			if(ioctl(_videoDev, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) < 0)
			{
				Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
				closeDevice(_videoDev);
			}
			else
			{
				if ( videoDisabled == 0 )
				{
					rc = true;
				}
			}
		}

	}
	return rc;
}

int AmlogicGrabber::grabFrame(Image<ColorRgb> & image)
{
	int rc = 0;
	if (_isEnabled && !_isDeviceInError)
	{
		// Make sure video is playing, else there is nothing to grab
		if (isVideoPlaying())
		{
			if (_grabbingModeNotification!=1)
			{
				Info(_log, "Switch to VPU capture mode");
				_grabbingModeNotification = 1;
				_lastError = 0;
			}

			if (grabFrame_amvideocap(image) < 0) {
				closeDevice(_captureDev);
				rc = -1;
			}
		}
		else
		{
			if (_grabbingModeNotification!=2)
			{
				Info( _log, "Switch to Framebuffer capture mode");
				_grabbingModeNotification = 2;
				_lastError = 0;
			}
			rc = _fbGrabber.grabFrame(image);
		}
	}
	return rc;
}

int AmlogicGrabber::grabFrame_amvideocap(Image<ColorRgb> & image)
{
	int rc = 0;

	// If the device is not open, attempt to open it
	if (_captureDev < 0)
	{
		if (! openDevice(_captureDev, DEFAULT_CAPTURE_DEVICE))
		{
			ErrorIf( _lastError != 1, _log,"Failed to open the AMLOGIC device (%d - %s):", errno, strerror(errno));
			_lastError = 1;
			rc = -1;
			return rc;
		}
	}

	long r1 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, _width);
	long r2 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, _height);
	long r3 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS, CAP_FLAG_AT_END);
	long r4 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WAIT_MAX_MS, AMVIDEOCAP_WAIT_MAX_MS);

	if (r1<0 || r2<0 || r3<0 || r4<0 || _height==0 || _width==0)
	{
		ErrorIf(_lastError != 2,_log,"Failed to configure capture device (%d - %s)", errno, strerror(errno));
		_lastError = 2;
		rc = -1;
	}
	else
	{
		int linelen = ((_width + 31) & ~31) * 3;
		size_t _bytesToRead = linelen * _height;

		// Read the snapshot into the memory
		ssize_t bytesRead   = pread(_captureDev, _image_ptr, _bytesToRead, 0);

		if ( bytesRead < 0 && !EAGAIN && errno > 0 )
		{
			ErrorIf(_lastError != 3, _log,"Capture frame failed  failed - Retrying. Error [%d] - %s", errno, strerror(errno));
			_lastError = 3;
			rc = -1;
		}
		else
		{
			if (bytesRead != -1 && static_cast<ssize_t>(_bytesToRead) != bytesRead)
			{
				// Read of snapshot failed
				ErrorIf(_lastError != 4, _log,"Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", _bytesToRead, bytesRead);
				_lastError = 4;
				rc = -1;
			}
			else {
				//If bytesRead = -1 but no error or EAGAIN or ENODATA, return last image to cover video pausing scenario
				// EAGAIN : // 11 - Resource temporarily unavailable
				// ENODATA: // 61 - No data available
				_imageResampler.processImage(static_cast<uint8_t*>(_image_ptr),
											  _width,
											  _height,
											  linelen,
											  PixelFormat::BGR24, image);
				_lastError = 0;
				rc = 0;
			}
		}
	}
	return rc;
}

QJsonObject AmlogicGrabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	QJsonObject inputsDiscovered;

	if(QFile::exists(DEFAULT_VIDEO_DEVICE) && QFile::exists(DEFAULT_CAPTURE_DEVICE) )
	{
		QJsonArray video_inputs;

		QSize screenSize = _fbGrabber.getScreenSize();
		if ( !screenSize.isEmpty() )
		{
			int fbIdx = _fbGrabber.getPath().right(1).toInt();

			DebugIf(verbose, _log, "FB device [%s] found with resolution: %dx%d", QSTRING_CSTR(_fbGrabber.getPath()), screenSize.width(), screenSize.height());
			QJsonArray fps = { 1, 5, 10, 15, 20, 25, 30};

			QJsonObject in;

			QString displayName;
			displayName = QString("Display%1").arg(fbIdx);

			in["name"] = displayName;
			in["inputIdx"] = fbIdx;

			QJsonArray formats;
			QJsonObject format;

			QJsonArray resolutionArray;

			QJsonObject resolution;

			resolution["width"] = screenSize.width();
			resolution["height"] = screenSize.height();
			resolution["fps"] = fps;

			resolutionArray.append(resolution);

			format["resolutions"] = resolutionArray;
			formats.append(format);

			in["formats"] = formats;
			video_inputs.append(in);
		}

		if (!video_inputs.isEmpty())
		{
			inputsDiscovered["device"] = "amlogic";
			inputsDiscovered["device_name"] = "AmLogic";
			inputsDiscovered["type"] = "screen";
			inputsDiscovered["video_inputs"] = video_inputs;

			QJsonObject defaults, video_inputs_default, resolution_default;
			resolution_default["fps"] = AMVIDEOCAP_DEFAULT_RATE_HZ;
			video_inputs_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			defaults["video_input"] = video_inputs_default;
			inputsDiscovered["default"] = defaults;
		}
	}

	if (inputsDiscovered.isEmpty())
	{
		DebugIf(verbose, _log, "No displays found to capture from!");
	}

	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}

void AmlogicGrabber::setVideoMode(VideoMode mode)
{
	Grabber::setVideoMode(mode);
	_fbGrabber.setVideoMode(mode);
}

bool AmlogicGrabber::setPixelDecimation(int pixelDecimation)
{
	return ( Grabber::setPixelDecimation( pixelDecimation) &&
			 _fbGrabber.setPixelDecimation( pixelDecimation));
}

void AmlogicGrabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	Grabber::setCropping(cropLeft, cropRight, cropTop, cropBottom);
	_fbGrabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

bool AmlogicGrabber::setWidthHeight(int width, int height)
{
	bool rc (false);
	if ( Grabber::setWidthHeight(width, height) )
	{
		_image_bgr.resize(static_cast<unsigned>(width), static_cast<unsigned>(height));
		_width = width;
		_height = height;
		_bytesToRead = _image_bgr.size();
		_image_ptr = _image_bgr.memptr();
		rc = _fbGrabber.setWidthHeight(width, height);
	}
	return rc;
}

bool AmlogicGrabber::setFramerate(int fps)
{
	return (Grabber::setFramerate(fps) &&
			 _fbGrabber.setFramerate(fps));
}
