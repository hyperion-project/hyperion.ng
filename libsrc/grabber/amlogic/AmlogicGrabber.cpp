#include <grabber/amlogic/AmlogicGrabber.h>

#include <algorithm>
#include <cassert>
#include <iostream>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include <dlfcn.h> // Required for dlopen, dlsym, dlclose

#include "Amvideocap.h"

#include <QFile>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDir>
#include <QSize>
#include <QScopedPointer>

// Local includes
#include <utils/Logger.h>

// Constants
namespace
{
	const int DEFAULT_DEVICE_IDX = 0;
	const char DEFAULT_VIDEO_DEVICE[] = "/dev/amvideo";
	const char DEFAULT_CAPTURE_DEVICE[] = "/dev/amvideocap0";
	// The Amlogic capture rate is driven by the video FPS. If video FPS is greater than 30hz, then it will be video FPS/2
	// The Hyperion rate is the frequence of frame grabs we attempt to do (but it is different from video FPS, as that is dynamic).
	const int AMVIDEOCAP_DEFAULT_RATE_HZ = 25; 
	const int AMVIDEOCAP_WAIT_MAX_MS = 100; // Max wait time for frame capture. A capture time-out or pause scenario are identified, if rate falls under 10 fps (100ms per frame).
	
	const size_t AMVIDEO_ALIGNMENT = 32; // Standard for Amlogic S905/S912
} // End of constants

int getAlignedSize(int size) {
    // For BGR24 (3 bytes per pixel) and 32-byte hardware alignment (AMVIDEO_ALIGNMENT)
    // We need (size * 3) to be a multiple of 32. 
    // The easiest way to ensure this is to make size a multiple of 32.
    int alignment = AMVIDEO_ALIGNMENT;
    return (size + alignment - 1) & ~(alignment - 1);
}

AmlogicGrabber::AmlogicGrabber(int deviceIdx)
	: Grabber("GRABBER-AMLOGIC") // Minimum required width or height is 160
	  , _captureDev(-1)
	  , _videoDev(-1)
	  , _lastError(0)
	  , _grabbingModeNotification(0)
{
	TRACK_SCOPE() << "Creating Amlogic grabber - deviceIdx:" << deviceIdx;	
	_image_ptr = _image_bgr.memptr();
	_useImageResampler = true;

	// Scaling will be done during grabbing
	_imageResampler.setPixelDecimation(1);
	//Disable Cropping
	_imageResampler.setCropping(0, 0, 0, 0);
}

AmlogicGrabber::~AmlogicGrabber()
{
	TRACK_SCOPE();
	closeDevice(_captureDev);
	closeDevice(_videoDev);
}

bool AmlogicGrabber::isGbmSupported() const
{
	// Check for the existence of gbm_create_device, a core GBM function, within libdrm.so

	QString libName = "libMali";
	QString lib = libName + ".so";

	// 1. Attempt to open the library.
	// RTLD_LAZY resolves symbols only when they are needed.
	void *handle = dlopen(QSTRING_CSTR(lib), RTLD_LAZY);

	if (!handle)
	{
		qCDebug(grabber_screen_properties) << "Could not check if DRM/GBM is supported. Error: " << dlerror()
										   << ". The GBM driver is not installed or not in the library path.";
		return false;
	}

	// 2. Look for a specific GBM function symbol.
	// We're not calling the function, just checking if its address exists.
	// 'gbm_create_device' is a fundamental function in the GBM API.
	const void *symbol = dlsym(handle, "gbm_create_device");

	// 3. Close the library handle.
	dlclose(handle);

	if (symbol != nullptr)
	{
		qCDebug(grabber_screen_properties) << "System likely supports DRM/GBM. Found 'gbm_create_device' in" << libName + ".so.";
		return true;
	}

	qCDebug(grabber_screen_properties) << "System likely does not support DRM/GBM. Could not find 'gbm_create_device' in" << libName + ".so.";
	return false;
}

bool AmlogicGrabber::openDevice(int &fd, const char *dev) const
{
	if (fd < 0)
	{
		fd = ::open(dev, O_RDWR);
		if (fd < 0)
		{
			return false;
		}
	}

	return true;
}

void AmlogicGrabber::closeDevice(int &fd) const
{
	if (fd >= 0)
	{
		::close(fd);
		fd = -1;
	}
}

bool AmlogicGrabber::isVideoPlaying()
{
	if (!QFile::exists(DEFAULT_VIDEO_DEVICE))
	{
		return false;
	}

	int videoDisabled{1};
	if (!openDevice(_videoDev, DEFAULT_VIDEO_DEVICE))
	{
		Error(_log, "Failed to open video device(%s): %d - %s", DEFAULT_VIDEO_DEVICE, errno, strerror(errno));
		return false;
	}

	// Check the video disabled flag
	if (ioctl(_videoDev, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) < 0)
	{
		Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
		closeDevice(_videoDev);
		return false;
	}

	if (videoDisabled == 0)
	{
		return true;
	}

	return false;
}

int AmlogicGrabber::grabFrame(Image<ColorRgb> &image)
{
	if (!_isEnabled || _isDeviceInError)
	{
		return -1;
	}

	// Make sure video is playing, else there is nothing to grab
	if (!isVideoPlaying())
	{
		if (_grabbingModeNotification == 2)
		{
			qCDebug(grabber_screen_capture_failed) << "No video is playing. No image captured from amlogic framebuffer.";
			return -1;
		}

		qCDebug(grabber_screen_flow) << "Video playing stopped. Stop VPU capture mode.";
		closeDevice(_captureDev);
		image.clear();
		_grabbingModeNotification = 2;
		_lastError = 0;
		return 0;
	}

	if (_grabbingModeNotification != 1)
	{
		qCDebug(grabber_screen_flow) << "Video is playing. Switch to VPU capture mode";
		_grabbingModeNotification = 1;
		_lastError = 0;
		return -1; // Skip the first frame after mode switch
	}

	int rc = grabFrame_amvideocap(image);
	if (rc < 0 )
	{
		if (_lastError != 0)
		{
			//Capture failed with error, error output already done in grabFrame_amvideocap
			closeDevice(_captureDev);
		}
		else
		{
			// No error, just try again
		}
		return -1;
	}

	return 0;
}

int AmlogicGrabber::grabFrame_amvideocap(Image<ColorRgb> &image)
{
	// If the device is not open, attempt to open it
	if (_captureDev < 0 && !openDevice(_captureDev, DEFAULT_CAPTURE_DEVICE))
	{
		ErrorIf(_lastError != 1, _log, "Failed to open the AMLOGIC device (%d - %s):", errno, strerror(errno));
		_lastError = 1;
		return -1;
	}

	long r1 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, _width);
	long r2 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, _height);
	long r3 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_AT_FLAGS, CAP_FLAG_AT_END);
	long r4 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WAIT_MAX_MS, AMVIDEOCAP_WAIT_MAX_MS);

	if (r1 < 0 || r2 < 0 || r3 < 0 || r4 < 0 || _height == 0 || _width == 0)
	{
		ErrorIf(_lastError != 2, _log, "Failed to configure capture device (%d - %s)", errno, strerror(errno));
		_lastError = 2;
		return -1;
	}

	// Read the snapshot into the memory
	auto bytesRead = pread(_captureDev, _image_ptr, _bytesToRead, 0);
	if (bytesRead < 0 )
	{
		//  EAGAIN : // 11 - Resource temporarily unavailable
		//  ENODATA: // 61 - No data available
		if (errno == EAGAIN || errno == ENODATA)
		{
			qCDebug(grabber_screen_capture_failed) << "No image captured. Captured frame is empty or device temporarily unavailable, retrying...";
			_lastError = 0;
			return -1;
		}

		//if (errno > 0)
		{
			ErrorIf(_lastError != 3, _log, "No image captured. Capture frame failed - Retrying. Error [%d] - %s", errno, strerror(errno));
			_lastError = 3;
			return -1;
		}
	}

	if (_bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		ErrorIf(_lastError != 4, _log, "No image captured. Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", _bytesToRead, bytesRead);
		_lastError = 4;
		return -1;
	}

	qCDebug(grabber_screen_capture) << "Captured image of size: " << _width << "x" << _height;

	_imageResampler.processImage(reinterpret_cast<uint8_t *>(_image_ptr),
								 _width,
								 _height,
								 _stride,
								 PixelFormat::BGR24,
								 image);
							 
	_lastError = 0;
	return 0;
}

bool AmlogicGrabber::setWidthHeight(int width, int height)
{
	qCDebug(grabber_screen_properties) << "Requested width: " << width << "height: " << height;

	width = width / _pixelDecimation;
	height = height / _pixelDecimation;
	// Disable scaling during during image processing, as captured image will already be in the correct size
	_imageResampler.setPixelDecimation(1);

	// Align width and height to hardware requirements
	width = getAlignedSize(width);
	height = getAlignedSize(height);
	if (!Grabber::setWidthHeight(width, height))
	{
		return false;
	}

	qCDebug(grabber_screen_properties) << "New safe image capture size : _width" << _width << "_height" << _height;

	_image_bgr.resize(static_cast<unsigned>(_width), static_cast<unsigned>(_height));
	_image_ptr = _image_bgr.memptr();

	_stride = _width * 3;
	_bytesToRead = _stride * _height;

	return true;
}

QJsonObject AmlogicGrabber::discover(const QJsonObject& /*params*/) const
{
	QJsonObject inputsDiscovered;

	QScopedPointer<Grabber> screenGrabber;
	if (isGbmSupported())
	{
		qCDebug(grabber_screen_properties) << "System supports DRM/GBM, using DRMFrameGrabber for screen capture.";
		screenGrabber.reset(new DRMFrameGrabber(DEFAULT_DEVICE_IDX));
	}
	else
	{
		qCDebug(grabber_screen_properties) << "DRM/GBM not supported, using FramebufferFrameGrabber for screen capture.";
		screenGrabber.reset(new FramebufferFrameGrabber(DEFAULT_DEVICE_IDX));
	}

	//Overwrite default supported fps values
	screenGrabber->setFpsSupported({ 1, 5, 10, 15, 20, 25, 30, 40, 50 });

	QJsonArray const video_inputs = screenGrabber->getInputDeviceDetails();
	if (video_inputs.isEmpty())
	{
		qCDebug(grabber_screen_properties) << "No displays found to capture from!";
		return {};
	}

	inputsDiscovered["device"] = "amlogic";
	inputsDiscovered["device_name"] = "AmLogic";
	inputsDiscovered["type"] = "screen";
	inputsDiscovered["video_inputs"] = video_inputs;

	QJsonObject defaults;
	QJsonObject video_inputs_default;
	QJsonObject resolution_default;

	resolution_default["fps"] = AMVIDEOCAP_DEFAULT_RATE_HZ;
	video_inputs_default["resolution"] = resolution_default;
	video_inputs_default["inputIdx"] = 0;
	defaults["video_input"] = video_inputs_default;
	inputsDiscovered["default"] = defaults;

	return inputsDiscovered;
}
