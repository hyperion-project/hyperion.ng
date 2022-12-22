#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <cstring>
#include <sstream>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include <hyperion/Hyperion.h>
#include <hyperion/HyperionIManager.h>

#include <QDirIterator>
#include <QFileInfo>
#include <QSet>

#include "grabber/V4L2Grabber.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

#ifndef V4L2_CAP_META_CAPTURE
	#define V4L2_CAP_META_CAPTURE 0x00800000 // Specified in kernel header v4.16. Required for backward compatibility.
#endif

// Constants
namespace { const bool verbose = false; }

// Need more video properties? Visit https://www.kernel.org/doc/html/v4.14/media/uapi/v4l/control.html
using ControlIDPropertyMap = QMap<unsigned int, QString>;
inline QMap<unsigned int, QString> initControlIDPropertyMap()
{
	QMap<unsigned int, QString> propertyMap
	{
		{V4L2_CID_BRIGHTNESS	, "brightness"	},
		{V4L2_CID_CONTRAST		, "contrast"	},
		{V4L2_CID_SATURATION	, "saturation"	},
		{V4L2_CID_HUE 			, "hue"			}
	};

	return propertyMap;
};

Q_GLOBAL_STATIC_WITH_ARGS(ControlIDPropertyMap, _controlIDPropertyMap, (initControlIDPropertyMap()));

static PixelFormat GetPixelFormat(const unsigned int format)
{
	if (format == V4L2_PIX_FMT_RGB32) return PixelFormat::RGB32;
	if (format == V4L2_PIX_FMT_RGB24) return PixelFormat::BGR24;
	if (format == V4L2_PIX_FMT_YUYV) return PixelFormat::YUYV;
	if (format == V4L2_PIX_FMT_UYVY) return PixelFormat::UYVY;
	if (format == V4L2_PIX_FMT_NV12) return  PixelFormat::NV12;
	if (format == V4L2_PIX_FMT_YUV420) return  PixelFormat::I420;
#ifdef HAVE_TURBO_JPEG
	if (format == V4L2_PIX_FMT_MJPEG) return  PixelFormat::MJPEG;
#endif
	return PixelFormat::NO_CHANGE;
};

V4L2Grabber::V4L2Grabber()
	: Grabber("V4L2")
	, _currentDevicePath("none")
	, _currentDeviceName("none")
	, _threadManager(nullptr)
	, _ioMethod(IO_METHOD_MMAP)
	, _fileDescriptor(-1)
	, _pixelFormat(PixelFormat::NO_CHANGE)
	, _pixelFormatConfig(PixelFormat::NO_CHANGE)
	, _lineLength(-1)
	, _frameByteSize(-1)
	, _currentFrame(0)
	, _noSignalCounterThreshold(40)
	, _noSignalThresholdColor(ColorRgb{0,0,0})
	, _cecDetectionEnabled(true)
	, _cecStandbyActivated(false)
	, _signalDetectionEnabled(true)
	, _noSignalDetected(false)
	, _noSignalCounter(0)
	, _brightness(0)
	, _contrast(0)
	, _saturation(0)
	, _hue(0)
	, _x_frac_min(0.25)
	, _y_frac_min(0.25)
	, _x_frac_max(0.75)
	, _y_frac_max(0.75)
	, _streamNotifier(nullptr)
	, _initialized(false)
	, _reload(false)
{
}

V4L2Grabber::~V4L2Grabber()
{
	uninit();

	if (_threadManager)
		delete _threadManager;
	_threadManager = nullptr;
}

bool V4L2Grabber::prepare()
{
	if (!_threadManager)
		_threadManager = new EncoderThreadManager(this);

	return (_threadManager != nullptr);
}

void V4L2Grabber::uninit()
{
	// stop if the grabber was not stopped
	if (_initialized)
	{
		Debug(_log,"Uninit grabber: %s (%s)", QSTRING_CSTR(_currentDeviceName), QSTRING_CSTR(_currentDevicePath));
		stop();
	}
}

bool V4L2Grabber::init()
{
	if (!_initialized)
	{
		bool noDevicePath = _currentDevicePath.compare("none", Qt::CaseInsensitive) == 0 || _currentDevicePath.compare("auto", Qt::CaseInsensitive) == 0;

		// enumerate the video capture devices on the user's system
		enumVideoCaptureDevices();

		if(noDevicePath)
			return false;

		if(!_deviceProperties.contains(_currentDevicePath))
		{
			Debug(_log, "Configured device at '%s' is not available.", QSTRING_CSTR(_currentDevicePath));
			_currentDevicePath = "none";
			return false;
		}
		else
		{
			if (HyperionIManager::getInstance())
				if (_currentDeviceName.compare("none", Qt::CaseInsensitive) == 0 || _currentDeviceName != _deviceProperties.value(_currentDevicePath).name)
					return false;

			Debug(_log, "Set device (path) to: %s (%s)", QSTRING_CSTR(_deviceProperties.value(_currentDevicePath).name), QSTRING_CSTR(_currentDevicePath));
		}

		// correct invalid parameters
		QMap<int, DeviceProperties::InputProperties>::const_iterator inputIterator = _deviceProperties.value(_currentDevicePath).inputs.find(_input);
		if (inputIterator == _deviceProperties.value(_currentDevicePath).inputs.end())
			setInput(_deviceProperties.value(_currentDevicePath).inputs.firstKey());

		QMultiMap<PixelFormat, DeviceProperties::InputProperties::EncodingProperties>::const_iterator encodingIterator = _deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.find(_pixelFormat);
		if (encodingIterator == _deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.end())
			setEncoding(pixelFormatToString(_deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.firstKey()));

		bool validDimensions = false;
		for (auto enc = _deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.constBegin(); enc != _deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.constEnd(); ++enc)
			if(enc.key() == _pixelFormat && enc.value().width == _width && enc.value().height == _height)
			{
				validDimensions = true;
				break;
			}

		if (!validDimensions)
			setWidthHeight(_deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.first().width, _deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.first().height);

		QList<int> availableframerates = _deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.value(_pixelFormat).framerates;
		if (!availableframerates.isEmpty() && !availableframerates.contains(_fps))
			setFramerate(_deviceProperties.value(_currentDevicePath).inputs.value(_input).encodingFormats.value(_pixelFormat).framerates.first());

		bool opened = false;
		try
		{
			if (open_device())
			{
				opened = true;
				init_device(_videoStandard);
				_initialized = true;
			}
		}
		catch(std::exception& e)
		{
			if (opened)
			{
				uninit_device();
				close_device();
			}

			Error(_log, "V4l2 init failed (%s)", e.what());
		}
	}

	return _initialized;
}

bool V4L2Grabber::start()
{
	try
	{
		if (init() && _streamNotifier != nullptr && !_streamNotifier->isEnabled())
		{
			connect(_threadManager, &EncoderThreadManager::newFrame, this, &V4L2Grabber::newThreadFrame);
			_threadManager->start();
			DebugIf(verbose, _log, "Decoding threads: %u", _threadManager->_threadCount);

			_streamNotifier->setEnabled(true);
			start_capturing();
			Info(_log, "Started");
			return true;
		}
	}
	catch(std::exception& e)
	{
		Error(_log, "start failed (%s)", e.what());
	}

	return false;
}

void V4L2Grabber::stop()
{
	if (_streamNotifier != nullptr && _streamNotifier->isEnabled())
	{
		_initialized = false;
		_threadManager->stop();
		disconnect(_threadManager, nullptr, nullptr, nullptr);
		stop_capturing();
		_streamNotifier->setEnabled(false);
		uninit_device();
		close_device();
		_deviceProperties.clear();
		_deviceControls.clear();
		Info(_log, "Stopped");
	}
}

bool V4L2Grabber::open_device()
{
	struct stat st;

	if (-1 == stat(QSTRING_CSTR(_currentDevicePath), &st))
	{
		throw_errno_exception("Cannot identify '" + _currentDevicePath + "'");
		return false;
	}

	if (!S_ISCHR(st.st_mode))
	{
		throw_exception("'" + _currentDevicePath + "' is no device");
		return false;
	}

	_fileDescriptor = open(QSTRING_CSTR(_currentDevicePath), O_RDWR | O_NONBLOCK, 0);

	if (-1 == _fileDescriptor)
	{
		throw_errno_exception("Cannot open '" + _currentDevicePath + "'");
		return false;
	}

	// create the notifier for when a new frame is available
	_streamNotifier = new QSocketNotifier(_fileDescriptor, QSocketNotifier::Read);
	_streamNotifier->setEnabled(false);
	connect(_streamNotifier, &QSocketNotifier::activated, this, &V4L2Grabber::read_frame);
	return true;
}

void V4L2Grabber::close_device()
{
	if (-1 == close(_fileDescriptor))
	{
		throw_errno_exception("close");
		return;
	}

	_fileDescriptor = -1;

	delete _streamNotifier;
	_streamNotifier = nullptr;
}

void V4L2Grabber::init_read(unsigned int buffer_size)
{
	_buffers.resize(1);

	_buffers[0].length = buffer_size;
	_buffers[0].start = malloc(buffer_size);

	if (!_buffers[0].start)
	{
		throw_exception("Out of memory");
		return;
	}
}

void V4L2Grabber::init_mmap()
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count = 4;
	req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_MMAP;

	if (-1 == xioctl(VIDIOC_REQBUFS, &req))
	{
		if (EINVAL == errno)
		{
			throw_exception("'" + _currentDevicePath + "' does not support memory mapping");
			return;
		}
		else
		{
			throw_errno_exception("VIDIOC_REQBUFS");
			return;
		}
	}

	if (req.count < 2)
	{
		throw_exception("Insufficient buffer memory on " + _currentDevicePath);
		return;
	}

	_buffers.resize(req.count);

	for (size_t n_buffers = 0; n_buffers < req.count; ++n_buffers)
	{
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type	= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	= V4L2_MEMORY_MMAP;
		buf.index	= n_buffers;

		if (-1 == xioctl(VIDIOC_QUERYBUF, &buf))
		{
			throw_errno_exception("VIDIOC_QUERYBUF");
			return;
		}

		_buffers[n_buffers].length = buf.length;
		_buffers[n_buffers].start = mmap(NULL /* start anywhere */,
						buf.length,
						PROT_READ | PROT_WRITE /* required */,
						MAP_SHARED /* recommended */,
						_fileDescriptor, buf.m.offset
					);

		if (MAP_FAILED == _buffers[n_buffers].start)
		{
			throw_errno_exception("mmap");
			return;
		}
	}
}

void V4L2Grabber::init_userp(unsigned int buffer_size)
{
	struct v4l2_requestbuffers req;

	CLEAR(req);

	req.count  = 4;
	req.type   = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	req.memory = V4L2_MEMORY_USERPTR;

	if (-1 == xioctl(VIDIOC_REQBUFS, &req))
	{
		if (EINVAL == errno)
		{
			throw_exception("'" + _currentDevicePath + "' does not support user pointer");
			return;
		}
		else
		{
			throw_errno_exception("VIDIOC_REQBUFS");
			return;
		}
	}

	_buffers.resize(4);

	for (size_t n_buffers = 0; n_buffers < 4; ++n_buffers)
	{
		_buffers[n_buffers].length = buffer_size;
		_buffers[n_buffers].start = malloc(buffer_size);

		if (!_buffers[n_buffers].start)
		{
			throw_exception("Out of memory");
			return;
		}
	}
}

void V4L2Grabber::init_device(VideoStandard videoStandard)
{
	struct v4l2_capability cap;
	CLEAR(cap);

	if (-1 == xioctl(VIDIOC_QUERYCAP, &cap))
	{
		if (EINVAL == errno)
		{
			throw_exception("'" + _currentDevicePath + "' is no V4L2 device");
			return;
		}
		else
		{
			throw_errno_exception("VIDIOC_QUERYCAP");
			return;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		throw_exception("'" + _currentDevicePath + "' is no video capture device");
		return;
	}

	switch (_ioMethod)
	{
		case IO_METHOD_READ:
		{
			if (!(cap.capabilities & V4L2_CAP_READWRITE))
			{
				throw_exception("'" + _currentDevicePath + "' does not support read i/o");
				return;
			}
		}
		break;

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
		{
			if (!(cap.capabilities & V4L2_CAP_STREAMING))
			{
				throw_exception("'" + _currentDevicePath + "' does not support streaming i/o");
				return;
			}
		}
		break;
	}

	/* Select video input, video standard and tune here. */

	struct v4l2_cropcap cropcap;
	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(VIDIOC_CROPCAP, &cropcap))
	{
		struct v4l2_crop crop;
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(VIDIOC_S_CROP, &crop))
		{
			switch (errno)
			{
				case EINVAL: /* Cropping not supported. */
				default: /* Errors ignored. */
					break;
			}
		}
	}

	// set input if needed and supported
	struct v4l2_input v4l2Input;
	CLEAR(v4l2Input);
	v4l2Input.index = _input;

	if (_input >= 0 && 0 == xioctl(VIDIOC_ENUMINPUT, &v4l2Input))
	{
		(-1 == xioctl(VIDIOC_S_INPUT, &_input))
		?	Debug(_log, "Input settings not supported.")
		:	Debug(_log, "Set device input to: %s", v4l2Input.name);
	}

	// set the video standard if needed and supported
	struct v4l2_standard standard;
	CLEAR(standard);

	if (-1 != xioctl(VIDIOC_ENUMSTD, &standard))
	{
		switch (videoStandard)
		{
			case VideoStandard::PAL:
			{
				standard.id = V4L2_STD_PAL;
				if (-1 == xioctl(VIDIOC_S_STD, &standard.id))
				{
					throw_errno_exception("VIDIOC_S_STD");
					break;
				}
				Debug(_log, "Video standard=PAL");
			}
			break;

			case VideoStandard::NTSC:
			{
				standard.id = V4L2_STD_NTSC;
				if (-1 == xioctl(VIDIOC_S_STD, &standard.id))
				{
					throw_errno_exception("VIDIOC_S_STD");
					break;
				}
				Debug(_log, "Video standard=NTSC");
			}
			break;

			case VideoStandard::SECAM:
			{
				standard.id = V4L2_STD_SECAM;
				if (-1 == xioctl(VIDIOC_S_STD, &standard.id))
				{
					throw_errno_exception("VIDIOC_S_STD");
					break;
				}
				Debug(_log, "Video standard=SECAM");
			}
			break;

			case VideoStandard::NO_CHANGE:
			default:
				// No change to device settings
				break;
		}
	}

	// get the current settings
	struct v4l2_format fmt;
	CLEAR(fmt);

	fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	if (-1 == xioctl(VIDIOC_G_FMT, &fmt))
	{
		throw_errno_exception("VIDIOC_G_FMT");
		return;
	}

	// set the requested pixel format
	switch (_pixelFormat)
	{
		case PixelFormat::RGB32:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
		break;

		case PixelFormat::BGR24:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB24;
		break;

		case PixelFormat::YUYV:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		break;

		case PixelFormat::UYVY:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
		break;

		case PixelFormat::NV12:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_NV12;
		break;

		case PixelFormat::I420:
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUV420;
		break;

#ifdef HAVE_TURBO_JPEG
		case PixelFormat::MJPEG:
		{
			fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_MJPEG;
			fmt.fmt.pix.field       = V4L2_FIELD_ANY;
		}
		break;
#endif

		case PixelFormat::NO_CHANGE:
		default:
			// No change to device settings
			break;
	}

	// set custom resolution for width and height if they are not zero
	if(_width != 0 && _height != 0)
	{
		fmt.fmt.pix.width = _width;
		fmt.fmt.pix.height = _height;
	}

	// set the settings
	if (-1 == xioctl(VIDIOC_S_FMT, &fmt))
	{
		throw_errno_exception("VIDIOC_S_FMT");
		return;
	}

	// initialize current width and height
	_width = fmt.fmt.pix.width;
	_height = fmt.fmt.pix.height;

	// display the used width and height
	Debug(_log, "Set resolution to width=%d height=%d", _width, _height );

	// Trying to set frame rate
	struct v4l2_streamparm streamparms;
	CLEAR(streamparms);

	streamparms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	// Check that the driver knows about framerate get/set
	if (xioctl(VIDIOC_G_PARM, &streamparms) >= 0)
	{
		// Check if the device is able to accept a capture framerate set.
		if (streamparms.parm.capture.capability == V4L2_CAP_TIMEPERFRAME)
		{
			streamparms.parm.capture.timeperframe.numerator = 1;
			streamparms.parm.capture.timeperframe.denominator = _fps;
			(-1 == xioctl(VIDIOC_S_PARM, &streamparms))
			?	Debug(_log, "Frame rate settings not supported.")
			:	Debug(_log, "Set framerate to %d fps", streamparms.parm.capture.timeperframe.denominator);
		}
	}

	// set the line length
	_lineLength = fmt.fmt.pix.bytesperline;

	// set brightness, contrast, saturation, hue
	for (auto control : _deviceControls[_currentDevicePath])
	{
		struct v4l2_control control_S;
		CLEAR(control_S);
		control_S.id = _controlIDPropertyMap->key(control.property);

		if (_controlIDPropertyMap->key(control.property) == V4L2_CID_BRIGHTNESS)
		{
			if (_brightness >= control.minValue && _brightness <= control.maxValue && _brightness != control.currentValue)
			{
				control_S.value = _brightness;
				if (xioctl(VIDIOC_S_CTRL, &control_S) >= 0)
					Debug(_log,"Set brightness to %i", _brightness);
			}
		}
		else if (_controlIDPropertyMap->key(control.property) == V4L2_CID_CONTRAST)
		{
			if (_contrast >= control.minValue && _contrast <= control.maxValue && _contrast != control.currentValue)
			{
				control_S.value = _contrast;
				if (xioctl(VIDIOC_S_CTRL, &control_S) >= 0)
					Debug(_log,"Set contrast to %i", _contrast);
			}
		}
		else if (_controlIDPropertyMap->key(control.property) == V4L2_CID_SATURATION)
		{
			if (_saturation >= control.minValue && _saturation <= control.maxValue && _saturation != control.currentValue)
			{
				control_S.value = _saturation;
				if (xioctl(VIDIOC_S_CTRL, &control_S) >= 0)
					Debug(_log,"Set saturation to %i", _saturation);
			}
		}
		else if (_controlIDPropertyMap->key(control.property) == V4L2_CID_HUE)
		{
			if (_hue >= control.minValue && _hue <= control.maxValue && _hue != control.currentValue)
			{
				control_S.value = _hue;
				if (xioctl(VIDIOC_S_CTRL, &control_S) >= 0)
					Debug(_log,"Set hue to %i", _hue);
			}
		}
	}

	// check pixel format and frame size
	switch (fmt.fmt.pix.pixelformat)
	{
		case V4L2_PIX_FMT_RGB32:
		{
			_pixelFormat = PixelFormat::RGB32;
			_frameByteSize = _width * _height * 4;
			Debug(_log, "Pixel format=RGB32");
		}
		break;

		case V4L2_PIX_FMT_RGB24:
		{
			_pixelFormat = PixelFormat::BGR24;
			_frameByteSize = _width * _height * 3;
			Debug(_log, "Pixel format=BGR24");
		}
		break;


		case V4L2_PIX_FMT_YUYV:
		{
			_pixelFormat = PixelFormat::YUYV;
			_frameByteSize = _width * _height * 2;
			Debug(_log, "Pixel format=YUYV");
		}
		break;

		case V4L2_PIX_FMT_UYVY:
		{
			_pixelFormat = PixelFormat::UYVY;
			_frameByteSize = _width * _height * 2;
			Debug(_log, "Pixel format=UYVY");
		}
		break;

		case V4L2_PIX_FMT_NV12:
		{
			_pixelFormat = PixelFormat::NV12;
			_frameByteSize = (_width * _height * 6) / 4;
			Debug(_log, "Pixel format=NV12");
		}
		break;

		case V4L2_PIX_FMT_YUV420:
		{
			_pixelFormat = PixelFormat::I420;
			_frameByteSize = (_width * _height * 6) / 4;
			Debug(_log, "Pixel format=I420");
		}
		break;

#ifdef HAVE_TURBO_JPEG
		case V4L2_PIX_FMT_MJPEG:
		{
			_pixelFormat = PixelFormat::MJPEG;
			Debug(_log, "Pixel format=MJPEG");
		}
		break;
#endif

		default:
#ifdef HAVE_TURBO_JPEG
			throw_exception("Only pixel formats RGB32, BGR24, YUYV, UYVY, NV12, I420 and MJPEG are supported");
#else
			throw_exception("Only pixel formats RGB32, BGR24, YUYV, UYVY, NV12 and I420 are supported");
#endif
		return;
	}

	switch (_ioMethod)
	{
		case IO_METHOD_READ:
			init_read(fmt.fmt.pix.sizeimage);
		break;

		case IO_METHOD_MMAP:
			init_mmap();
		break;

		case IO_METHOD_USERPTR:
			init_userp(fmt.fmt.pix.sizeimage);
		break;
	}
}

void V4L2Grabber::uninit_device()
{
	switch (_ioMethod)
	{
		case IO_METHOD_READ:
			free(_buffers[0].start);
		break;

		case IO_METHOD_MMAP:
		{
			for (size_t i = 0; i < _buffers.size(); ++i)
				if (-1 == munmap(_buffers[i].start, _buffers[i].length))
				{
					throw_errno_exception("munmap");
					return;
				}
		}
		break;

		case IO_METHOD_USERPTR:
		{
			for (size_t i = 0; i < _buffers.size(); ++i)
				free(_buffers[i].start);
		}
		break;
	}

	_buffers.resize(0);
}

void V4L2Grabber::start_capturing()
{
	switch (_ioMethod)
	{
		case IO_METHOD_READ:
			/* Nothing to do. */
			break;

		case IO_METHOD_MMAP:
		{
			for (size_t i = 0; i < _buffers.size(); ++i)
			{
				struct v4l2_buffer buf;

				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;
				buf.index = i;

				if (-1 == xioctl(VIDIOC_QBUF, &buf))
				{
					throw_errno_exception("VIDIOC_QBUF");
					return;
				}
			}
			v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl(VIDIOC_STREAMON, &type))
			{
				throw_errno_exception("VIDIOC_STREAMON");
				return;
			}
			break;
		}
		case IO_METHOD_USERPTR:
		{
			for (size_t i = 0; i < _buffers.size(); ++i)
			{
				struct v4l2_buffer buf;

				CLEAR(buf);
				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;
				buf.index = i;
				buf.m.userptr = (unsigned long)_buffers[i].start;
				buf.length = _buffers[i].length;

				if (-1 == xioctl(VIDIOC_QBUF, &buf))
				{
					throw_errno_exception("VIDIOC_QBUF");
					return;
				}
			}
			v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (-1 == xioctl(VIDIOC_STREAMON, &type))
			{
				throw_errno_exception("VIDIOC_STREAMON");
				return;
			}
			break;
		}
	}
}

void V4L2Grabber::stop_capturing()
{
	enum v4l2_buf_type type;

	switch (_ioMethod)
	{
		case IO_METHOD_READ:
			break; /* Nothing to do. */

		case IO_METHOD_MMAP:
		case IO_METHOD_USERPTR:
		{
			type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			ErrorIf((xioctl(VIDIOC_STREAMOFF, &type) == -1), _log, "VIDIOC_STREAMOFF  error code  %d, %s", errno, strerror(errno));
		}
		break;
	}
}

int V4L2Grabber::read_frame()
{
	bool rc = false;

	try
	{
		struct v4l2_buffer buf;

		switch (_ioMethod)
		{
			case IO_METHOD_READ:
			{
				int size;
				if ((size = read(_fileDescriptor, _buffers[0].start, _buffers[0].length)) == -1)
				{
					switch (errno)
					{
						case EAGAIN:
							return 0;

						case EIO: /* Could ignore EIO, see spec. */
						default:
							throw_errno_exception("read");
						return 0;
					}
				}

				rc = process_image(_buffers[0].start, size);
			}
			break;

			case IO_METHOD_MMAP:
			{
				CLEAR(buf);

				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_MMAP;

				if (-1 == xioctl(VIDIOC_DQBUF, &buf))
				{
					switch (errno)
					{
						case EAGAIN:
							return 0;

						case EIO: /* Could ignore EIO, see spec. */
						default:
						{
							throw_errno_exception("VIDIOC_DQBUF");
							stop();
							enumVideoCaptureDevices();
						}
						return 0;
					}
				}

				assert(buf.index < _buffers.size());

				rc = process_image(_buffers[buf.index].start, buf.bytesused);

				if (-1 == xioctl(VIDIOC_QBUF, &buf))
				{
					throw_errno_exception("VIDIOC_QBUF");
					return 0;
				}
			}
			break;

			case IO_METHOD_USERPTR:
			{
				CLEAR(buf);

				buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				buf.memory = V4L2_MEMORY_USERPTR;

				if (-1 == xioctl(VIDIOC_DQBUF, &buf))
				{
					switch (errno)
					{
						case EAGAIN:
							return 0;

						case EIO: /* Could ignore EIO, see spec. */
						default:
						{
							throw_errno_exception("VIDIOC_DQBUF");
							stop();
							enumVideoCaptureDevices();
						}
						return 0;
					}
				}

				for (size_t i = 0; i < _buffers.size(); ++i)
				{
					if (buf.m.userptr == (unsigned long)_buffers[i].start && buf.length == _buffers[i].length)
					{
						break;
					}
				}

				rc = process_image((void *)buf.m.userptr, buf.bytesused);

				if (!rc && -1 == xioctl(VIDIOC_QBUF, &buf))
				{
					throw_errno_exception("VIDIOC_QBUF");
					return 0;
				}
			}
			break;
		}
	}
	catch (std::exception& e)
	{
		emit readError(e.what());
		rc = false;
	}

	return rc ? 1 : 0;
}

bool V4L2Grabber::process_image(const void *p, int size)
{
	int processFrameIndex = _currentFrame++, result = false;

	// frame skipping
	if ((processFrameIndex % (_fpsSoftwareDecimation + 1) != 0) && (_fpsSoftwareDecimation > 0))
		return result;

#ifdef HAVE_TURBO_JPEG
	if (size < _frameByteSize && _pixelFormat != PixelFormat::MJPEG)
#else
	if (size < _frameByteSize)
#endif
	{
		Error(_log, "Frame too small: %d != %d", size, _frameByteSize);
	}
	else if (_threadManager != nullptr)
	{
		for (int i = 0; i < _threadManager->_threadCount; i++)
		{
			if (!_threadManager->_threads[i]->isBusy())
			{
				_threadManager->_threads[i]->setup(_pixelFormat, (uint8_t*)p, size, _width, _height, _lineLength, _cropLeft, _cropTop, _cropBottom, _cropRight, _videoMode, _flipMode, _pixelDecimation);
				_threadManager->_threads[i]->process();
				result = true;
				break;
			}
		}
	}

	return result;
}

void V4L2Grabber::newThreadFrame(Image<ColorRgb> image)
{
	if (_cecDetectionEnabled && _cecStandbyActivated)
		return;

	if (_signalDetectionEnabled)
	{
		// check signal (only in center of the resulting image, because some grabbers have noise values along the borders)
		bool noSignal = true;

		// top left
		unsigned xOffset  = image.width()  * _x_frac_min;
		unsigned yOffset  = image.height() * _y_frac_min;

		// bottom right
		unsigned xMax     = image.width()  * _x_frac_max;
		unsigned yMax     = image.height() * _y_frac_max;

		for (unsigned x = xOffset; noSignal && x < xMax; ++x)
			for (unsigned y = yOffset; noSignal && y < yMax; ++y)
				noSignal &= (ColorRgb&)image(x, y) <= _noSignalThresholdColor;

		if (noSignal)
			++_noSignalCounter;
		else
		{
			if (_noSignalCounter >= _noSignalCounterThreshold)
			{
				_noSignalDetected = true;
				Info(_log, "Signal detected");
			}

			_noSignalCounter = 0;
		}

		if ( _noSignalCounter < _noSignalCounterThreshold)
		{
			emit newFrame(image);
		}
		else if (_noSignalCounter == _noSignalCounterThreshold)
		{
			_noSignalDetected = false;
			Info(_log, "Signal lost");
		}
	}
	else
		emit newFrame(image);
}

int V4L2Grabber::xioctl(int request, void *arg)
{
	int r;

	do
	{
		r = ioctl(_fileDescriptor, request, arg);
	}
	while (-1 == r && EINTR == errno);

	return r;
}

int V4L2Grabber::xioctl(int fileDescriptor, int request, void *arg)
{
	int r;

	do
	{
		r = ioctl(fileDescriptor, request, arg);
	}
	while (r < 0 && errno == EINTR );

	return r;
}

void V4L2Grabber::setDevice(const QString& devicePath, const QString& deviceName)
{
	if (_currentDevicePath != devicePath || _currentDeviceName != deviceName)
	{
		_currentDevicePath = devicePath;
		_currentDeviceName = deviceName;
		_reload = true;
	}
}

bool V4L2Grabber::setInput(int input)
{
	if(Grabber::setInput(input))
	{
		_reload = true;
		return true;
	}

	 return false;
}

bool V4L2Grabber::setWidthHeight(int width, int height)
{
	if(Grabber::setWidthHeight(width, height))
	{
		_reload = true;
		return true;
	}

	 return false;
}

void V4L2Grabber::setEncoding(QString enc)
{
	if(_pixelFormatConfig != parsePixelFormat(enc))
	{
		_pixelFormatConfig = parsePixelFormat(enc);
		if(_initialized)
		{
			Debug(_log,"Set hardware encoding to: %s", QSTRING_CSTR(enc.toUpper()));
			_reload = true;
		}
		else
			_pixelFormat = _pixelFormatConfig;
	}
}

void V4L2Grabber::setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue)
{
	if (_brightness != brightness || _contrast != contrast || _saturation != saturation || _hue != hue)
	{
		_brightness = brightness;
		_contrast = contrast;
		_saturation = saturation;
		_hue = hue;

		_reload = true;
	}
}

void V4L2Grabber::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold)
{
	_noSignalThresholdColor.red   = uint8_t(255*redSignalThreshold);
	_noSignalThresholdColor.green = uint8_t(255*greenSignalThreshold);
	_noSignalThresholdColor.blue  = uint8_t(255*blueSignalThreshold);
	_noSignalCounterThreshold     = qMax(1, noSignalCounterThreshold);

	if(_signalDetectionEnabled)
		Info(_log, "Signal threshold set to: {%d, %d, %d}", _noSignalThresholdColor.red, _noSignalThresholdColor.green, _noSignalThresholdColor.blue );
}

void V4L2Grabber::setSignalDetectionOffset(double horizontalMin, double verticalMin, double horizontalMax, double verticalMax)
{
	// rainbow 16 stripes 0.47 0.2 0.49 0.8
	// unicolor: 0.25 0.25 0.75 0.75

	_x_frac_min = horizontalMin;
	_y_frac_min = verticalMin;
	_x_frac_max = horizontalMax;
	_y_frac_max = verticalMax;

	if(_signalDetectionEnabled)
		Info(_log, "Signal detection area set to: %f,%f x %f,%f", _x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max );
}

void V4L2Grabber::setSignalDetectionEnable(bool enable)
{
	if (_signalDetectionEnabled != enable)
	{
		_signalDetectionEnabled = enable;
		if(_initialized)
			Info(_log, "Signal detection is now %s", enable ? "enabled" : "disabled");
	}
}

void V4L2Grabber::setCecDetectionEnable(bool enable)
{
	if (_cecDetectionEnabled != enable)
	{
		_cecDetectionEnabled = enable;
		if(_initialized)
			Info(_log, "%s", QSTRING_CSTR(QString("CEC detection is now %1").arg(enable ? "enabled" : "disabled")));
	}
}

bool V4L2Grabber::reload(bool force)
{
	if (_reload || force)
	{
		if (_streamNotifier != nullptr && _streamNotifier->isEnabled())
		{
			Info(_log,"Reloading V4L2 Grabber");
			uninit();
			_pixelFormat = _pixelFormatConfig;
		}

		_reload = false;
		return prepare() && start();
	}

	return false;
}

#if defined(ENABLE_CEC)

void V4L2Grabber::handleCecEvent(CECEvent event)
{
	switch (event)
	{
		case CECEvent::On  :
			Debug(_log,"CEC on event received");
			_cecStandbyActivated = false;
			return;
		case CECEvent::Off :
			Debug(_log,"CEC off event received");
			_cecStandbyActivated = true;
			return;
		default: break;
	}
}

#endif

QJsonArray V4L2Grabber::discover(const QJsonObject& params)
{
	DebugIf(verbose, _log, "params: [%s]", QString(QJsonDocument(params).toJson(QJsonDocument::Compact)).toUtf8().constData());

	enumVideoCaptureDevices();

	QJsonArray inputsDiscovered;
	for (auto device_property = _deviceProperties.constBegin(); device_property != _deviceProperties.constEnd(); ++device_property)
	{
		QJsonObject device, in;
		QJsonArray video_inputs, formats;

		if (!device_property.value().inputs.isEmpty())
		{
			device["device"] = device_property.key();
			device["device_name"] = device_property.value().name;
			device["type"] = "v4l2";

			for (auto input = device_property.value().inputs.constBegin(); input != device_property.value().inputs.constEnd(); ++input)
			{
				in["name"] = input.value().inputName;
				in["inputIdx"] = input.key();

				QJsonArray standards;
				for (auto std = input.value().standards.constBegin(); std != input.value().standards.constEnd(); ++std)
					if(!standards.contains(VideoStandard2String(*std)))
						standards.append(VideoStandard2String(*std));

				if (!standards.isEmpty())
					in["standards"] = standards;

				for (auto encodingFormat : input.value().encodingFormats.uniqueKeys())
				{
					QJsonObject format;
					QJsonArray resolutionArray;

					format["format"] = pixelFormatToString(encodingFormat);

					QMap<std::pair<int, int>, QSet<int>> combined = QMap<std::pair<int, int>, QSet<int>>();
					for (auto enc : input.value().encodingFormats.values(encodingFormat))
					{
						std::pair<int, int> width_height{enc.width, enc.height};
						auto &com = combined[width_height];
						for (auto framerate : qAsConst(enc.framerates))
						{
							com.insert(framerate);
						}
					}

					for (auto enc = combined.constBegin(); enc != combined.constEnd(); ++enc)
					{
						QJsonObject resolution;
						QJsonArray fps;

						resolution["width"] = enc.key().first;
						resolution["height"] = enc.key().second;

						for (auto framerate : enc.value())
							fps.append(framerate);

						resolution["fps"] = fps;
						resolutionArray.append(resolution);
					}

					format["resolutions"] = resolutionArray;
					formats.append(format);
				}
				in["formats"] = formats;
				video_inputs.append(in);

			}

			device["video_inputs"] = video_inputs;

			QJsonObject controls, controls_default;
			for (const auto &control : qAsConst(_deviceControls[device_property.key()]))
			{
				QJsonObject property;
				property["minValue"] = control.minValue;
				property["maxValue"] = control.maxValue;
				property["step"] = control.step;
				property["current"] = control.currentValue;
				controls[control.property] = property;
				controls_default[control.property] = control.defaultValue;
			}
			device["properties"] = controls;

			QJsonObject defaults, video_inputs_default, format_default, resolution_default;
			resolution_default["width"] = 640;
			resolution_default["height"] = 480;
			resolution_default["fps"] = 25;
			format_default["format"] = "yuyv";
			format_default["resolution"] = resolution_default;
			video_inputs_default["inputIdx"] = 0;
			video_inputs_default["standards"] = "PAL";
			video_inputs_default["formats"] = format_default;

			defaults["video_input"] = video_inputs_default;
			defaults["properties"] = controls_default;
			device["default"] = defaults;

			inputsDiscovered.append(device);
		}
	}

	_deviceProperties.clear();
	_deviceControls.clear();
	DebugIf(verbose, _log, "device: [%s]", QString(QJsonDocument(inputsDiscovered).toJson(QJsonDocument::Compact)).toUtf8().constData());

	return inputsDiscovered;
}

void V4L2Grabber::enumVideoCaptureDevices()
{
	QDirIterator it("/sys/class/video4linux/", QDirIterator::NoIteratorFlags);
	_deviceProperties.clear();
	_deviceControls.clear();

	while(it.hasNext())
	{
		//_v4lDevices
		QString dev = it.next();
		if (it.fileName().startsWith("video"))
		{
			QString devName = "/dev/" + it.fileName();
			int fd = open(QSTRING_CSTR(devName), O_RDWR | O_NONBLOCK, 0);

			if (fd < 0)
			{
				throw_errno_exception("Cannot open '" + devName + "'");
				continue;
			}

			struct v4l2_capability cap;
			CLEAR(cap);

			if (xioctl(fd, VIDIOC_QUERYCAP, &cap) < 0)
			{
				throw_errno_exception("'" + devName + "' is no V4L2 device");
				close(fd);
				continue;
			}

			if (cap.device_caps & V4L2_CAP_META_CAPTURE) // this device has bit 23 set (and bit 1 reset), so it doesn't have capture.
			{
				close(fd);
				continue;
			}

			// get the current settings
			struct v4l2_format fmt;
			CLEAR(fmt);

			fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
			if (xioctl(fd, VIDIOC_G_FMT, &fmt) < 0)
			{
				close(fd);
				continue;
			}

			V4L2Grabber::DeviceProperties properties;

			// collect available device inputs (index & name)
			struct v4l2_input input;
			CLEAR(input);

			input.index = 0;
			while (xioctl(fd, VIDIOC_ENUMINPUT, &input) >= 0)
			{
				V4L2Grabber::DeviceProperties::InputProperties inputProperties;
				inputProperties.inputName = QString((char*)input.name);

				// Enumerate video standards
				struct v4l2_standard standard;
				CLEAR(standard);

				standard.index = 0;
				while (xioctl(fd, VIDIOC_ENUMSTD, &standard) >= 0)
				{
					if (standard.id & input.std)
					{
						if (standard.id == V4L2_STD_PAL)
							inputProperties.standards.append(VideoStandard::PAL);
						else if (standard.id == V4L2_STD_NTSC)
							inputProperties.standards.append(VideoStandard::NTSC);
						else if (standard.id == V4L2_STD_SECAM)
							inputProperties.standards.append(VideoStandard::SECAM);
					}

					standard.index++;
				}

				// Enumerate pixel formats
				struct v4l2_fmtdesc desc;
				CLEAR(desc);

				desc.index = 0;
				desc.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
				while (xioctl(fd, VIDIOC_ENUM_FMT, &desc) == 0)
				{
					PixelFormat encodingFormat = GetPixelFormat(desc.pixelformat);
					if (encodingFormat != PixelFormat::NO_CHANGE)
					{
						V4L2Grabber::DeviceProperties::InputProperties::EncodingProperties encodingProperties;

						// Enumerate frame sizes and frame rates
						struct v4l2_frmsizeenum frmsizeenum;
						CLEAR(frmsizeenum);

						frmsizeenum.index = 0;
						frmsizeenum.pixel_format = desc.pixelformat;
						while (xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum) >= 0)
						{
							switch (frmsizeenum.type)
							{
								case V4L2_FRMSIZE_TYPE_DISCRETE:
								{
									encodingProperties.width	= frmsizeenum.discrete.width;
									encodingProperties.height	= frmsizeenum.discrete.height;
									enumFrameIntervals(encodingProperties.framerates, fd, desc.pixelformat, frmsizeenum.discrete.width, frmsizeenum.discrete.height);
								}
								break;
								case V4L2_FRMSIZE_TYPE_CONTINUOUS:
								case V4L2_FRMSIZE_TYPE_STEPWISE: // We do not take care of V4L2_FRMSIZE_TYPE_CONTINUOUS or V4L2_FRMSIZE_TYPE_STEPWISE
									break;
							}

							inputProperties.encodingFormats.insert(encodingFormat, encodingProperties);
							frmsizeenum.index++;
						}

						// Failsafe: In case VIDIOC_ENUM_FRAMESIZES fails, insert current heigth, width and fps.
						if (xioctl(fd, VIDIOC_ENUM_FRAMESIZES, &frmsizeenum) == -1)
						{
							encodingProperties.width	= fmt.fmt.pix.width;
							encodingProperties.height	= fmt.fmt.pix.height;
							enumFrameIntervals(encodingProperties.framerates, fd, desc.pixelformat, encodingProperties.width, encodingProperties.height);
							inputProperties.encodingFormats.insert(encodingFormat, encodingProperties);
						}
					}

					desc.index++;
				}

				properties.inputs.insert(input.index, inputProperties);
				input.index++;
			}

			// Enumerate video control IDs
			QList<DeviceControls> deviceControlList;
			for (auto itDeviceControls = _controlIDPropertyMap->constBegin(); itDeviceControls != _controlIDPropertyMap->constEnd(); itDeviceControls++)
			{
				struct v4l2_queryctrl queryctrl;
				CLEAR(queryctrl);

				queryctrl.id = itDeviceControls.key();
				if (xioctl(fd, VIDIOC_QUERYCTRL, &queryctrl) < 0)
					break;
				if (queryctrl.flags & V4L2_CTRL_FLAG_DISABLED)
					break;

				DeviceControls control;
				control.property = itDeviceControls.value();
				control.minValue = queryctrl.minimum;
				control.maxValue = queryctrl.maximum;
				control.step = queryctrl.step;
				control.defaultValue = queryctrl.default_value;

				struct v4l2_ext_control ctrl;
				struct v4l2_ext_controls ctrls;
				CLEAR(ctrl);
				CLEAR(ctrls);

				ctrl.id = itDeviceControls.key();
				ctrls.count = 1;
				ctrls.controls = &ctrl;
				if (xioctl(fd, VIDIOC_G_EXT_CTRLS, &ctrls) == 0)
				{
					control.currentValue = ctrl.value;
					DebugIf(verbose, _log, "%s: min=%i, max=%i, step=%i, default=%i, current=%i", QSTRING_CSTR(itDeviceControls.value()), control.minValue, control.maxValue, control.step, control.defaultValue, control.currentValue);
				}
				else
					break;

				deviceControlList.append(control);
			}

			if (!deviceControlList.isEmpty())
				_deviceControls.insert("/dev/"+it.fileName(), deviceControlList);

			if (close(fd) < 0) continue;

			QFile devNameFile(dev+"/name");
			if (devNameFile.exists())
			{
				devNameFile.open(QFile::ReadOnly);
				devName = devNameFile.readLine();
				devName = devName.trimmed();
				properties.name = devName;
				devNameFile.close();
			}

			_deviceProperties.insert("/dev/"+it.fileName(), properties);
		}
	}
}

void V4L2Grabber::enumFrameIntervals(QList<int> &framerates, int fileDescriptor, int pixelformat, int width, int height)
{
	// collect available frame rates
	struct v4l2_frmivalenum frmivalenum;
	CLEAR(frmivalenum);

	frmivalenum.index = 0;
	frmivalenum.pixel_format = pixelformat;
	frmivalenum.width = width;
	frmivalenum.height = height;

	while (xioctl(fileDescriptor, VIDIOC_ENUM_FRAMEINTERVALS, &frmivalenum) >= 0)
	{
		int rate;
		switch (frmivalenum.type)
		{
			case V4L2_FRMSIZE_TYPE_DISCRETE:
			{
				if (frmivalenum.discrete.numerator != 0)
				{
					rate = frmivalenum.discrete.denominator / frmivalenum.discrete.numerator;
					if (!framerates.contains(rate))
						framerates.append(rate);
				}
			}
			break;
			case V4L2_FRMSIZE_TYPE_CONTINUOUS:
			case V4L2_FRMSIZE_TYPE_STEPWISE:
			{
				if (frmivalenum.stepwise.min.denominator != 0)
				{
					rate = frmivalenum.stepwise.min.denominator / frmivalenum.stepwise.min.numerator;
					if (!framerates.contains(rate))
						framerates.append(rate);
				}
			}
		}
		frmivalenum.index++;
	}

	// If VIDIOC_ENUM_FRAMEINTERVALS fails, try to read the current fps via VIDIOC_G_PARM if possible and insert it into 'framerates'.
	if (xioctl(fileDescriptor, VIDIOC_ENUM_FRAMESIZES, &frmivalenum) == -1)
	{
		struct v4l2_streamparm streamparms;
		CLEAR(streamparms);
		streamparms.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

		if (xioctl(fileDescriptor, VIDIOC_G_PARM, &streamparms) >= 0)
			framerates.append(streamparms.parm.capture.timeperframe.denominator / streamparms.parm.capture.timeperframe.numerator);
	}
}
