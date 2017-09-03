// STL includes
#include <algorithm>
#include <cassert>
#include <iostream>
#include <QFile>

// Linux includes
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>

// Local includes
#include <utils/Logger.h>
#include <grabber/AmlogicGrabber.h>
#include "Amvideocap.h"

#define VIDEO_DEVICE   "/dev/amvideo"
#define CAPTURE_DEVICE "/dev/amvideocap0"
#define GE2D_DEVICE    "/dev/ge2d"

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height)
	: Grabber("AMLOGICGRABBER", qMax(160u, width), qMax(160u, height)) // Minimum required width or height is 160
	, _captureDev(-1)
	, _videoDev(-1)
	, _ge2dDev(-1)
	, _lastError(0)
	, _fbGrabber("/dev/fb0",width,height)
	, _grabbingModeNotification(0)
	, _ge2dAvailable(true)
	, _ge2dVideoBufferPtr(nullptr)
	, _ge2dIonBuffer(nullptr)
{
	Debug(_log, "constructed(%d x %d)",_width,_height);
}

AmlogicGrabber::~AmlogicGrabber()
{
	closeDev(_captureDev);
	closeDev(_videoDev);
	closeDev(_ge2dDev);
}

bool AmlogicGrabber::openDev(int &fd, const char* dev)
{
	if (fd<0)
	{
		fd = open(dev, O_RDWR);
	}
	return fd >= 0;
}

void AmlogicGrabber::closeDev(int &fd)
{
	if (fd >= 0)
	{
		close(fd);
		fd = -1;
	}
}

bool AmlogicGrabber::isVideoPlaying()
{
	if(!QFile::exists(VIDEO_DEVICE)) return false;

	int videoDisabled = 1;
	if (!openDev(_videoDev, VIDEO_DEVICE))
	{
		Error(_log, "Failed to open video device(%s): %d - %s", VIDEO_DEVICE, errno, strerror(errno));
		return false;
	}
	else
	{
		// Check the video disabled flag
		if(ioctl(_videoDev, AMSTREAM_IOC_GET_VIDEO_DISABLE, &videoDisabled) < 0)
		{
			Error(_log, "Failed to retrieve video state from device: %d - %s", errno, strerror(errno));
			closeDev(_videoDev);
			return false;
		}
	}

	return videoDisabled == 0;
}

int AmlogicGrabber::grabFrame(Image<ColorRgb> & image)
{
	if (!_enabled) return 0;

	image.resize(_width,_height);
	// Make sure video is playing, else there is nothing to grab
	if (isVideoPlaying())
	{
		if (_grabbingModeNotification!=1)
		{
			Info(_log, "VPU mode");
			_grabbingModeNotification = 1;
			_lastError = 0;
		}

		if (_ge2dAvailable)
		{
			try
			{
				_ge2dAvailable = (QFile::exists(GE2D_DEVICE) && grabFrame_ge2d(image) == 0);
			}
			catch (...)
			{
				_ge2dAvailable = false;
			}

			if (!_ge2dAvailable)
			{
				closeDev(_videoDev);
				closeDev(_ge2dDev);
				Warning(_log, "GE2D capture interface not available! try Amvideocap instead");
			}
		}
		else if (QFile::exists(CAPTURE_DEVICE))
		{
			grabFrame_amvideocap(image);
		}
	}
	else
	{
		if (_grabbingModeNotification!=2)
		{
			Info( _log, "FB mode");
			_grabbingModeNotification = 2;
			_lastError = 0;
		}
		_fbGrabber.grabFrame(image);
	}

	closeDev(_videoDev);

	return 0;
}


int AmlogicGrabber::grabFrame_amvideocap(Image<ColorRgb> & image)
{
	// If the device is not open, attempt to open it
	if (! openDev(_captureDev, CAPTURE_DEVICE))
	{
		ErrorIf( _lastError != 1, _log,"Failed to open the AMLOGIC device (%d - %s):", errno, strerror(errno));
		_lastError = 1;
		return -1;
	}

	long r1 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_WIDTH, _width);
	long r2 = ioctl(_captureDev, AMVIDEOCAP_IOW_SET_WANTFRAME_HEIGHT, _height);

	if (r1<0 || r2<0 || _height==0 || _width==0)
	{
		ErrorIf(_lastError != 2,_log,"Failed to configure capture size (%d - %s)", errno, strerror(errno));
		closeDev(_captureDev);
		_lastError = 2;
		return -1;
	}

	// Read the snapshot into the memory
	image.resize(_width, _height);
	_image_bgr.resize(_width, _height);
	const ssize_t bytesToRead = _image_bgr.size();
	void * image_ptr          = _image_bgr.memptr();
	const ssize_t bytesRead   = pread(_captureDev, image_ptr, bytesToRead, 0);

	if (bytesRead < 0)
	{
		ErrorIf(_lastError != 3, _log,"Read of device failed: %d - %s", errno, strerror(errno));
		closeDev(_captureDev);
		_lastError = 3;
		return -1;
	}
	else if (bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		ErrorIf(_lastError != 4, _log,"Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", bytesToRead, bytesRead);
		closeDev(_captureDev);
		return -1;
	}

	closeDev(_captureDev);
	_useImageResampler = true;
	_imageResampler.processImage((const uint8_t*)image_ptr, _width, _height, _width*3, PIXELFORMAT_BGR24, image);
	_lastError = 0;

	return 0;
}


int AmlogicGrabber::grabFrame_ge2d(Image<ColorRgb> & image)
{
	if ( ! openDev(_ge2dDev, GE2D_DEVICE) || ! openDev(_videoDev, VIDEO_DEVICE))
	{
		Error(_log, "cannot open devices");
		return -1;
	}
	
	// Ion
	if (_ge2dIonBuffer == nullptr)
	{
		_ge2dIonBuffer = new IonBuffer(_width * _height * 3); // BGR
		_ge2dVideoBufferPtr = _ge2dIonBuffer->Map();
		memset(_ge2dVideoBufferPtr, 0, _ge2dIonBuffer->BufferSize());
	}

	int canvas_index;
	if (ioctl(_videoDev, AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME, &canvas_index) < 0)
	{
		Error(_log, "AMSTREAM_EXT_GET_CURRENT_VIDEOFRAME failed.");
		return -1;
	}

	uint32_t canvas0addr;

	if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR, &canvas0addr) < 0)
	{
		Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR failed.");
		return -1;
	}

	uint32_t ge2dformat;
	if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT, &ge2dformat) <0)
	{
		Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT failed.");
		return -1;
	}

	uint64_t size;
	if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_SIZE, &size) < 0)
	{
		Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_SIZE failed.");
		return -1;
	}

	unsigned cropLeft    = _cropLeft;
	unsigned cropRight   = _cropRight;
	unsigned cropTop     = _cropTop;
	unsigned cropBottom  = _cropBottom;
	int videoWidth       = (size >> 32) - cropLeft - cropRight;
	int videoHeight      = (size & 0xffffff) - cropTop - cropBottom;
	
	// calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
	case VIDEO_3DSBS:
		videoWidth /= 2;
		cropLeft /= 2;
		break;
	case VIDEO_3DTAB:
		videoHeight /= 2;
		cropTop /= 2;
		break;
	case VIDEO_2D:
	default:
		break;
	}

	struct config_para_ex_s configex = { 0 };
	configex.src_para.mem_type = CANVAS_TYPE_INVALID;
	configex.src_para.canvas_index = canvas0addr;
	configex.src_para.left     = cropLeft;
	configex.src_para.top      = cropTop;
	configex.src_para.width    = videoWidth;
	configex.src_para.height   = videoHeight / 2;
	configex.src_para.format   = ge2dformat;

	configex.dst_para.mem_type = CANVAS_ALLOC;
	configex.dst_para.format   = GE2D_FORMAT_S24_RGB;
	configex.dst_para.left     = 0;
	configex.dst_para.top      = 0;
	configex.dst_para.width    = _width;
	configex.dst_para.height   = _height;

	configex.dst_planes[0].addr = (long unsigned int)_ge2dIonBuffer->PhysicalAddress();
	configex.dst_planes[0].w    = configex.dst_para.width;
	configex.dst_planes[0].h    = configex.dst_para.height;

	if (ioctl(_ge2dDev, GE2D_CONFIG_EX, &configex) < 0)
	{
		Error(_log, "video GE2D_CONFIG_EX failed.");
		return -1;
	}

	ge2d_para_s blitRect = { 0 };
	blitRect.src1_rect.x = 0;
	blitRect.src1_rect.y = 0;
	blitRect.src1_rect.w = configex.src_para.width;
	blitRect.src1_rect.h = configex.src_para.height;

	blitRect.dst_rect.x = 0;
	blitRect.dst_rect.y = 0;
	blitRect.dst_rect.w = configex.dst_para.width ;
	blitRect.dst_rect.h = configex.dst_para.height;

	// Blit to videoBuffer
	if (ioctl(_ge2dDev, GE2D_STRETCHBLIT_NOALPHA, &blitRect) < 0)
	{
		Error(_log,"GE2D_STRETCHBLIT_NOALPHA failed.");
		return -1;
	}

	// Return video frame
	if (ioctl(_videoDev, AMVIDEO_EXT_PUT_CURRENT_VIDEOFRAME) < 0)
	{
		Error(_log, "AMSTREAM_EXT_PUT_CURRENT_VIDEOFRAME failed.");
		return -1;
	}

	_ge2dIonBuffer->Sync();

	// Read the snapshot into the memory
	_useImageResampler = false;
	_imageResampler.processImage((const uint8_t*)_ge2dVideoBufferPtr, _width, _height, _width*3, PIXELFORMAT_BGR24, image);

	closeDev(_videoDev);
	closeDev(_ge2dDev);

	return 0;
}

