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

AmlogicGrabber::AmlogicGrabber(const unsigned width, const unsigned height, const unsigned ge2d_mode, const QString device)
	: Grabber("AMLOGICGRABBER", qMax(160u, width), qMax(160u, height)) // Minimum required width or height is 160
	, _captureDev(-1)
	, _videoDev(-1)
	, _ge2dDev(-1)
	, _lastError(0)
	, _fbGrabber("/dev/fb0",width,height)
	, _grabbingModeNotification(0)
	, _ge2dVideoBufferPtr(nullptr)
	, _ge2dIonBuffer(nullptr)
	, _ge2d_mode(ge2d_mode)
	, _device(device)
{
	Debug(_log, "constructed(%d x %d), grabber device: %s",_width,_height,QSTRING_CSTR(_device));

	if (_device.contains("ge2d"))
		Debug(_log, "'ge2d' device use mode %d",ge2d_mode);

	_image_bgr.resize(_width, _height);
	_bytesToRead = _image_bgr.size();
	_image_ptr = _image_bgr.memptr();
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

	// Make sure video is playing, else there is nothing to grab
	if (isVideoPlaying())
	{
		if (_grabbingModeNotification!=1)
		{
			Info(_log, "VPU mode");
			_grabbingModeNotification = 1;
			_lastError = 0;
		}

		if (_device == "ge2d")
		{
			if (grabFrame_ge2d(image) < 0)
			{
				closeDev(_videoDev);
				closeDev(_ge2dDev);
			}
		}
		else if (_device == "amvideocap0")
		{
			if (grabFrame_amvideocap(image) < 0)
			{
				closeDev(_videoDev);
				closeDev(_captureDev);
			}
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

	return 0;
}


int AmlogicGrabber::grabFrame_amvideocap(Image<ColorRgb> & image)
{
	// If the device is not open, attempt to open it
	if (_captureDev < 0)
	{
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
	}

	// Read the snapshot into the memory
	ssize_t bytesRead   = pread(_captureDev, _image_ptr, _bytesToRead, 0);

	if (bytesRead < 0)
	{
		ErrorIf(_lastError != 3, _log,"Read of device failed: %d - %s", errno, strerror(errno));
		closeDev(_captureDev);
		_lastError = 3;
		return -1;
	}
	else if (_bytesToRead != bytesRead)
	{
		// Read of snapshot failed
		ErrorIf(_lastError != 4, _log,"Capture failed to grab entire image [bytesToRead(%d) != bytesRead(%d)]", _bytesToRead, bytesRead);
		closeDev(_captureDev);
		_lastError = 4;
		return -1;
	}

	_useImageResampler = true;
	_imageResampler.processImage((const uint8_t*)_image_ptr, _width, _height, (_width << 1) + _width, PIXELFORMAT_BGR24, image);
	_lastError = 0;

	return 0;
}


int AmlogicGrabber::grabFrame_ge2d(Image<ColorRgb> & image)
{
	int ret;
	static int videoWidth;
	static int videoHeight;

	if ( ! openDev(_ge2dDev, GE2D_DEVICE) || ! openDev(_videoDev, VIDEO_DEVICE))
	{
		ErrorIf( _lastError != 1 && _ge2dDev < 0, _log,"Failed to open the ge2d device: (%d - %s)", errno, strerror(errno));
		ErrorIf( _lastError != 1 && _videoDev < 0, _log,"Failed to open the AMLOGIC video device: (%d - %s)", errno, strerror(errno));
		_lastError = 1;
		return -1;
	}

	// Ion
	if (_ge2dIonBuffer == nullptr)
	{
		_ge2dIonBuffer = new IonBuffer(_width * _height * 3); // BGR
		_ge2dVideoBufferPtr = _ge2dIonBuffer->Map();
		memset(_ge2dVideoBufferPtr, 0, _ge2dIonBuffer->BufferSize());

		memset(&_configex, 0, sizeof(_configex));
		_configex.src_para.mem_type = CANVAS_TYPE_INVALID;
		_configex.dst_para.mem_type = CANVAS_ALLOC;
		_configex.dst_para.format   = GE2D_FORMAT_S24_RGB;
		_configex.dst_planes[0].addr = (long unsigned int)_ge2dIonBuffer->PhysicalAddress();
		_configex.dst_para.width    = _width;
		_configex.dst_para.height   = _height;
		_configex.dst_planes[0].w    = _configex.dst_para.width;
		_configex.dst_planes[0].h    = _configex.dst_para.height;

		memset(&_blitRect, 0, sizeof(_blitRect));
		_blitRect.dst_rect.w = _configex.dst_para.width;
		_blitRect.dst_rect.h = _configex.dst_para.height;
	}

	switch(_ge2d_mode)
	{
		case ge2d_single:
			{
				int canvas_index;
				if ((ret = ioctl(_videoDev, AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME, &canvas_index)) < 0)
				{
					if (ret != -EAGAIN)
					{
						Error(_log, "AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME failed: (%d - %s)", errno, strerror(errno));
					}
					else
					{
						Warning(_log, "AMVIDEO_EXT_GET_CURRENT_VIDEOFRAME failed, please try again!");
					}
					return -1;
				}

				uint32_t canvas0addr;
				if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR, &canvas0addr) < 0)
				{
					Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_CANVAS0ADDR failed: (%d - %s)", errno, strerror(errno));
					return -1;
				}

				_configex.src_para.canvas_index = canvas0addr;

				uint32_t ge2dformat;
				if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT, &ge2dformat) <0)
				{
					Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_GE2D_FORMAT failed: (%d - %s)", errno, strerror(errno));
					return -1;
				}

				_configex.src_para.format = ge2dformat;

				uint64_t size;
				if (ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_SIZE, &size) < 0)
				{
					Error(_log, "AMSTREAM_EXT_CURRENT_VIDEOFRAME_GET_SIZE failed: (%d - %s)", errno, strerror(errno));
					return -1;
				}

				videoWidth = (size >> 32) - _cropLeft - _cropRight;
				videoHeight = (size & 0xffffff) - _cropTop - _cropBottom;
			}
			break;

		case ge2d_combined:
			{
				static struct amvideo_grabber_data grabber_data;

				if ((ret = ioctl(_videoDev, AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_DATA, &grabber_data)) < 0)
				{
					if (ret == -EAGAIN)
					{
						Warning(_log, "AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_DATA failed, please try again!");
						return 0;
					}
					else
					{
						Error(_log, "AMVIDEO_EXT_CURRENT_VIDEOFRAME_GET_DATA failed.");
						return -1;
					}
				}

				videoWidth = (grabber_data.size >> 32) - _cropLeft - _cropRight;
				videoHeight = (grabber_data.size & 0xffffff) - _cropTop - _cropBottom;

				_configex.src_para.canvas_index = grabber_data.canvas0Addr;
				_configex.src_para.format = grabber_data.ge2dformat;
			}
			break;
	}

	unsigned cropLeft    = _cropLeft;
	unsigned cropTop     = _cropTop;
	
	// calculate final image dimensions and adjust top/left cropping in 3D modes
	switch (_videoMode)
	{
	case VIDEO_3DSBS:
		videoWidth >>= 1;
		cropLeft = _cropLeft >> 1;
		break;
	case VIDEO_3DTAB:
		videoHeight >>= 1;
		cropTop = _cropTop >> 1;
		break;
	case VIDEO_2D:
	default:
		break;
	}

	_configex.src_para.left     = cropLeft;
	_configex.src_para.top      = cropTop;
	_configex.src_para.width    = videoWidth;
	_configex.src_para.height   = videoHeight >> 1;

	if (ioctl(_ge2dDev, GE2D_CONFIG_EX, &_configex) < 0)
	{
		Error(_log, "video GE2D_CONFIG_EX failed.");
		return -1;
	}

	_blitRect.src1_rect.w = _configex.src_para.width;
	_blitRect.src1_rect.h = _configex.src_para.height;

	// Blit to videoBuffer
	if (ioctl(_ge2dDev, GE2D_STRETCHBLIT_NOALPHA, &_blitRect) < 0)
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
	_imageResampler.processImage((const uint8_t*)_ge2dVideoBufferPtr, _width, _height, (_width << 1) + _width, PIXELFORMAT_BGR24, image);
	_lastError = 0;

	return 0;
}

