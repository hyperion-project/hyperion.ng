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

#include <QDirIterator>
#include <QFileInfo>

#include "grabber/V4L2Grabber.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

V4L2Grabber::V4L2Grabber(const QString & device
		, int input
		, VideoStandard videoStandard
		, PixelFormat pixelFormat
		, int pixelDecimation
		)
	: Grabber("V4L2:"+device)
	, _deviceName(device)
	, _input(input)
	, _videoStandard(videoStandard)
	, _ioMethod(IO_METHOD_MMAP)
	, _fileDescriptor(-1)
	, _buffers()
	, _pixelFormat(pixelFormat)
	, _pixelDecimation(pixelDecimation)
	, _lineLength(-1)
	, _frameByteSize(-1)
	, _noSignalCounterThreshold(50)
	, _noSignalThresholdColor(ColorRgb{0,0,0})
	, _signalDetectionEnabled(true)
	, _noSignalDetected(false)
	, _noSignalCounter(0)
	, _x_frac_min(0.25)
	, _y_frac_min(0.25)
	, _x_frac_max(0.75)
	, _y_frac_max(0.75)
	, _streamNotifier(nullptr)
	, _initialized(false)
	, _deviceAutoDiscoverEnabled(false)

{
	//_imageResampler.setHorizontalPixelDecimation(pixelDecimation);
	//_imageResampler.setVerticalPixelDecimation(pixelDecimation);

	getV4Ldevices();
}

V4L2Grabber::~V4L2Grabber()
{
	uninit();
}

void V4L2Grabber::uninit()
{
	Debug(_log,"uninit grabber: %s", QSTRING_CSTR(_deviceName));
	// stop if the grabber was not stopped
	if (_initialized)
	{
		stop();
		uninit_device();
		close_device();
		_initialized = false;
	}
}


bool V4L2Grabber::init()
{
	if (! _initialized)
	{
		getV4Ldevices();
		QString v4lDevices_str;

		// show list only once
		if ( ! QString(QSTRING_CSTR(_deviceName)).startsWith("/dev/") )
		{
			for (auto& dev: _v4lDevices)
			{
				v4lDevices_str += "\t"+ dev.first + "\t" + dev.second + "\n";
			}
			Info(_log, "available V4L2 devices:\n%s", QSTRING_CSTR(v4lDevices_str));
		}

		if ( _deviceName == "auto" )
		{
			_deviceAutoDiscoverEnabled = true;
			_deviceName = "unknown";
			Info( _log, "search for usable video devices" );
			for (auto& dev: _v4lDevices)
			{
				_deviceName = dev.first;
				if ( init() )
				{
					Info(_log, "found usable v4l2 device: %s (%s)",QSTRING_CSTR(dev.first), QSTRING_CSTR(dev.second));
					_deviceAutoDiscoverEnabled = false;
					return _initialized;
				}
			}
			Info( _log, "no usable device found" );
		}
		else if ( ! _deviceName.startsWith("/dev/") )
		{
			for (auto& dev: _v4lDevices)
			{
				if ( _deviceName.toLower() == dev.second.toLower() )
				{
					_deviceName = dev.first;
					Info(_log, "found v4l2 device with configured name: %s (%s)", QSTRING_CSTR(dev.second), QSTRING_CSTR(dev.first) );
					break;
				}
			}
		}
		else
		{
			Info(_log, "%s v4l device: %s", (_deviceAutoDiscoverEnabled? "test" : "configured"), QSTRING_CSTR(_deviceName));
		}

		bool opened = false;
		try
		{
			open_device();
			opened = true;
			init_device(_videoStandard, _input);
			_initialized = true;
		}
		catch(std::exception& e)
		{
			if (opened)
			{
				uninit_device();
				close_device();
			}
			ErrorIf( !_deviceAutoDiscoverEnabled, _log, "V4l2 init failed (%s)", e.what());
		}
	}

	return _initialized;
}

void V4L2Grabber::getV4Ldevices()
{
	QDirIterator it("/sys/class/video4linux/", QDirIterator::NoIteratorFlags);
	while(it.hasNext())
	{
		//_v4lDevices
		QString dev = it.next();
		if (it.fileName().startsWith("video"))
		{
			QFile devNameFile(dev+"/name");
			QString devName;
			if ( devNameFile.exists())
			{
				devNameFile.open(QFile::ReadOnly);
				devName = devNameFile.readLine();
				devName = devName.trimmed();
				devNameFile.close();
			}
			_v4lDevices.emplace("/dev/"+it.fileName(), devName);
		}
    }
}

void V4L2Grabber::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold)
{
	_noSignalThresholdColor.red   = uint8_t(255*redSignalThreshold);
	_noSignalThresholdColor.green = uint8_t(255*greenSignalThreshold);
	_noSignalThresholdColor.blue  = uint8_t(255*blueSignalThreshold);
	_noSignalCounterThreshold     = qMax(1, noSignalCounterThreshold);

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

	Info(_log, "Signal detection area set to: %f,%f x %f,%f", _x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max );
}

QRectF V4L2Grabber::getSignalDetectionOffset()
{
	return QRectF(_x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max);
}

bool V4L2Grabber::start()
{
	try
	{
		if (init() && _streamNotifier != nullptr && !_streamNotifier->isEnabled())
		{
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
		stop_capturing();
		_streamNotifier->setEnabled(false);
		Info(_log, "Stopped");
	}
}

void V4L2Grabber::open_device()
{
	struct stat st;

	if (-1 == stat(QSTRING_CSTR(_deviceName), &st))
	{
		throw_errno_exception("Cannot identify '" + _deviceName + "'");
		return;
	}

	if (!S_ISCHR(st.st_mode))
	{
		throw_exception("'" + _deviceName + "' is no device");
		return;
	}

	_fileDescriptor = open(QSTRING_CSTR(_deviceName), O_RDWR | O_NONBLOCK, 0);

	if (-1 == _fileDescriptor)
	{
		throw_errno_exception("Cannot open '" + _deviceName + "'");
		return;
	}

	// create the notifier for when a new frame is available
	_streamNotifier = new QSocketNotifier(_fileDescriptor, QSocketNotifier::Read);
	_streamNotifier->setEnabled(false);
	connect(_streamNotifier, SIGNAL(activated(int)), this, SLOT(read_frame()));
}

void V4L2Grabber::close_device()
{
	if (-1 == close(_fileDescriptor))
	{
		throw_errno_exception("close");
		return;
	}

	_fileDescriptor = -1;

	if (_streamNotifier != nullptr)
	{
		delete _streamNotifier;
		_streamNotifier = nullptr;
	}
}

void V4L2Grabber::init_read(unsigned int buffer_size)
{
	_buffers.resize(1);

	_buffers[0].length = buffer_size;
	_buffers[0].start = malloc(buffer_size);

	if (!_buffers[0].start) {
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

	if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno) {
			throw_exception("'" + _deviceName + "' does not support memory mapping");
			return;
		} else {
			throw_errno_exception("VIDIOC_REQBUFS");
			return;
		}
	}

	if (req.count < 2) {
		throw_exception("Insufficient buffer memory on " + _deviceName);
		return;
	}

	_buffers.resize(req.count);

	for (size_t n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type		= V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory	  = V4L2_MEMORY_MMAP;
		buf.index	   = n_buffers;

		if (-1 == xioctl(VIDIOC_QUERYBUF, &buf))
		{
			throw_errno_exception("VIDIOC_QUERYBUF");
			return;
		}

		_buffers[n_buffers].length = buf.length;
		_buffers[n_buffers].start =
				mmap(NULL /* start anywhere */,
					 buf.length,
					 PROT_READ | PROT_WRITE /* required */,
					 MAP_SHARED /* recommended */,
					 _fileDescriptor, buf.m.offset);

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

	if (-1 == xioctl(VIDIOC_REQBUFS, &req)) {
		if (EINVAL == errno)
		{
			throw_exception("'" + _deviceName + "' does not support user pointer");
			return;
		} else {
			throw_errno_exception("VIDIOC_REQBUFS");
			return;
		}
	}

	_buffers.resize(4);

	for (size_t n_buffers = 0; n_buffers < 4; ++n_buffers) {
		_buffers[n_buffers].length = buffer_size;
		_buffers[n_buffers].start = malloc(buffer_size);

		if (!_buffers[n_buffers].start) {
			throw_exception("Out of memory");
			return;
		}
	}
}

void V4L2Grabber::init_device(VideoStandard videoStandard, int input)
{
	struct v4l2_capability cap;
	if (-1 == xioctl(VIDIOC_QUERYCAP, &cap))
	{
		if (EINVAL == errno) {
			throw_exception("'" + _deviceName + "' is no V4L2 device");
			return;
		} else {
			throw_errno_exception("VIDIOC_QUERYCAP");
			return;
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		throw_exception("'" + _deviceName + "' is no video capture device");
		return;
	}

	switch (_ioMethod) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE))
		{
			throw_exception("'" + _deviceName + "' does not support read i/o");
			return;
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING))
		{
			throw_exception("'" + _deviceName + "' does not support streaming i/o");
			return;
		}
		break;
	}


	/* Select video input, video standard and tune here. */

	struct v4l2_cropcap cropcap;
	CLEAR(cropcap);

	cropcap.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	if (0 == xioctl(VIDIOC_CROPCAP, &cropcap)) {
		struct v4l2_crop crop;
		crop.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		crop.c = cropcap.defrect; /* reset to default */

		if (-1 == xioctl(VIDIOC_S_CROP, &crop)) {
			switch (errno) {
			case EINVAL:
				/* Cropping not supported. */
				break;
			default:
				/* Errors ignored. */
				break;
			}
		}
	} else {
		/* Errors ignored. */
	}

	// set input if needed
	if (input >= 0)
	{
		if (-1 == xioctl(VIDIOC_S_INPUT, &input))
		{
			throw_errno_exception("VIDIOC_S_INPUT");
			return;
		}
	}

	// set the video standard if needed
	switch (videoStandard)
	{
	case VIDEOSTANDARD_PAL:
	{
		v4l2_std_id std_id = V4L2_STD_PAL;
		if (-1 == xioctl(VIDIOC_S_STD, &std_id))
		{
			throw_errno_exception("VIDIOC_S_STD");
			return;
		}
	}
		break;
	case VIDEOSTANDARD_NTSC:
	{
		v4l2_std_id std_id = V4L2_STD_NTSC;
		if (-1 == xioctl(VIDIOC_S_STD, &std_id))
		{
			throw_errno_exception("VIDIOC_S_STD");
			return;
		}
	}
		break;
	case VIDEOSTANDARD_SECAM:
	{
		v4l2_std_id std_id = V4L2_STD_SECAM;
		if (-1 == xioctl(VIDIOC_S_STD, &std_id))
		{
			throw_errno_exception("VIDIOC_S_STD");
			return;
		}
	}
		break;
	case VIDEOSTANDARD_NO_CHANGE:
	default:
		// No change to device settings
		break;
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
	case PIXELFORMAT_UYVY:
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_UYVY;
		break;
	case PIXELFORMAT_YUYV:
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_YUYV;
		break;
	case PIXELFORMAT_RGB32:
		fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB32;
		break;
	case PIXELFORMAT_NO_CHANGE:
	default:
		// No change to device settings
		break;
	}

	// calc the size based on pixelDecimation
	fmt.fmt.pix.width = fmt.fmt.pix.width / _pixelDecimation;
	fmt.fmt.pix.height = fmt.fmt.pix.height / _pixelDecimation;

	// set the line length
	_lineLength = fmt.fmt.pix.bytesperline;

	// set the settings
	if (-1 == xioctl(VIDIOC_S_FMT, &fmt))
	{
		throw_errno_exception("VIDIOC_S_FMT");
		return;
	}

	// get the format settings again
	// (the size may not have been accepted without an error)
	if (-1 == xioctl(VIDIOC_G_FMT, &fmt))
	{
		throw_errno_exception("VIDIOC_G_FMT");
		return;
	}

	// store width & height
	_width = fmt.fmt.pix.width;
	_height = fmt.fmt.pix.height;

	// display the used width and height
	Debug(_log, "width=%d height=%d", _width, _height );


	// check pixel format and frame size
	switch (fmt.fmt.pix.pixelformat)
	{
	case V4L2_PIX_FMT_UYVY:
		_pixelFormat = PIXELFORMAT_UYVY;
		_frameByteSize = _width * _height * 2;
		Debug(_log, "Pixel format=UYVY");
		break;
	case V4L2_PIX_FMT_YUYV:
		_pixelFormat = PIXELFORMAT_YUYV;
		_frameByteSize = _width * _height * 2;
		Debug(_log, "Pixel format=YUYV");
		break;
	case V4L2_PIX_FMT_RGB32:
		_pixelFormat = PIXELFORMAT_RGB32;
		_frameByteSize = _width * _height * 4;
		Debug(_log, "Pixel format=RGB32");
		break;
	default:
		throw_exception("Only pixel formats UYVY, YUYV, and RGB32 are supported");
		return;
	}

	switch (_ioMethod) {
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
	switch (_ioMethod) {
	case IO_METHOD_READ:
		free(_buffers[0].start);
		break;

	case IO_METHOD_MMAP:
		for (size_t i = 0; i < _buffers.size(); ++i)
			if (-1 == munmap(_buffers[i].start, _buffers[i].length))
			{
				throw_errno_exception("munmap");
				return;
			}
		break;

	case IO_METHOD_USERPTR:
		for (size_t i = 0; i < _buffers.size(); ++i)
			free(_buffers[i].start);
		break;
	}

	_buffers.resize(0);
}

void V4L2Grabber::start_capturing()
{
	switch (_ioMethod) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	{
		for (size_t i = 0; i < _buffers.size(); ++i) {
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
		for (size_t i = 0; i < _buffers.size(); ++i) {
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

	switch (_ioMethod) {
	case IO_METHOD_READ:
		/* Nothing to do. */
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		ErrorIf((xioctl(VIDIOC_STREAMOFF, &type) == -1), _log, "VIDIOC_STREAMOFF  error code  %d, %s", errno, strerror(errno));
		break;
	}
}

int V4L2Grabber::read_frame()
{
	bool rc = false;

	try
	{
	struct v4l2_buffer buf;

	switch (_ioMethod) {
	case IO_METHOD_READ:
		int size;
		if ((size = read(_fileDescriptor, _buffers[0].start, _buffers[0].length)) == -1)
		{
			switch (errno)
			{
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				throw_errno_exception("read");
				return 0;
			}
		}

		rc = process_image(_buffers[0].start, size);
		break;

	case IO_METHOD_MMAP:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_MMAP;

		if (-1 == xioctl(VIDIOC_DQBUF, &buf))
		{
			switch (errno)
			{
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				throw_errno_exception("VIDIOC_DQBUF");
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

		break;

	case IO_METHOD_USERPTR:
		CLEAR(buf);

		buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory = V4L2_MEMORY_USERPTR;

		if (-1 == xioctl(VIDIOC_DQBUF, &buf))
		{
			switch (errno)
			{
			case EAGAIN:
				return 0;

			case EIO:
				/* Could ignore EIO, see spec. */

				/* fall through */

			default:
				throw_errno_exception("VIDIOC_DQBUF");
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

		if (-1 == xioctl(VIDIOC_QBUF, &buf))
		{
			throw_errno_exception("VIDIOC_QBUF");
			return 0;
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
	// We do want a new frame...
	if (size != _frameByteSize)
	{
		Error(_log, "Frame too small: %d != %d", size, _frameByteSize);
	}
	else
	{
		process_image(reinterpret_cast<const uint8_t *>(p));
		return true;
	}

	return false;
}

void V4L2Grabber::process_image(const uint8_t * data)
{
	Image<ColorRgb> image(0, 0);
	_imageResampler.processImage(data, _width, _height, _lineLength, _pixelFormat, image);

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
		{
			for (unsigned y = yOffset; noSignal && y < yMax; ++y)
			{
				noSignal &= (ColorRgb&)image(x, y) <= _noSignalThresholdColor;
			}
		}

		if (noSignal)
		{
			++_noSignalCounter;
		}
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
	{
		emit newFrame(image);
	}
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

void V4L2Grabber::throw_exception(const QString & error)
{
	Error(_log, "Throws error: %s", QSTRING_CSTR(error));
}

void V4L2Grabber::throw_errno_exception(const QString & error)
{
	Error(_log, "Throws error nr: %s", QSTRING_CSTR(QString(error + " error code " + QString::number(errno) + ", " + strerror(errno))));
}

void V4L2Grabber::setSignalDetectionEnable(bool enable)
{
	if(_signalDetectionEnabled != enable)
	{
		_signalDetectionEnabled = enable;
		Info(_log, "Signal detection is now %s", enable ? "enabled" : "disabled");
	}
}

bool V4L2Grabber::getSignalDetectionEnabled()
{
	return _signalDetectionEnabled;
}

void V4L2Grabber::setPixelDecimation(int pixelDecimation)
{
	if(_pixelDecimation != pixelDecimation)
	{
		uninit();
		init();
	}
}

void V4L2Grabber::setInputVideoStandard(int input, VideoStandard videoStandard)
{
	if(_input != input || _videoStandard != videoStandard)
	{
		_input = input;
		_videoStandard = videoStandard;
		uninit();
		init();
	}
}
