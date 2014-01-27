#include <iostream>
#include <sstream>
#include <stdexcept>
#include <cstdio>
#include <cassert>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/videodev2.h>

#include "V4L2Grabber.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

static inline uint8_t clamp(int x)
{
	return (x<0) ? 0 : ((x>255) ? 255 : uint8_t(x));
}

static void yuv2rgb(uint8_t y, uint8_t u, uint8_t v, uint8_t & r, uint8_t & g, uint8_t & b)
{
	// see: http://en.wikipedia.org/wiki/YUV#Y.27UV444_to_RGB888_conversion
	int c = y - 16;
	int d = u - 128;
	int e = v - 128;

	r = clamp((298 * c + 409 * e + 128) >> 8);
	g = clamp((298 * c - 100 * d - 208 * e + 128) >> 8);
	b = clamp((298 * c + 516 * d + 128) >> 8);
}


V4L2Grabber::V4L2Grabber(const std::string &device, int input, VideoStandard videoStandard, int width, int height, int cropHorizontal, int cropVertical, int frameDecimation, int pixelDecimation) :
	_deviceName(device),
	_ioMethod(IO_METHOD_MMAP),
	_fileDescriptor(-1),
	_buffers(),
	_pixelFormat(0),
	_width(width),
	_height(height),
	_cropWidth(cropHorizontal),
	_cropHeight(cropVertical),
	_frameDecimation(std::max(1, frameDecimation)),
	_pixelDecimation(std::max(1, pixelDecimation)),
	_currentFrame(0),
	_callback(nullptr),
	_callbackArg(nullptr)
{
	open_device();
	init_device(videoStandard, input);
}

V4L2Grabber::~V4L2Grabber()
{
	uninit_device();
	close_device();
}

void V4L2Grabber::setCallback(V4L2Grabber::ImageCallback callback, void *arg)
{
	_callback = callback;
	_callbackArg = arg;
}

void V4L2Grabber::start()
{
	start_capturing();
}

void V4L2Grabber::capture(int frameCount)
{
	for (int count = 0; count < frameCount || frameCount < 0; ++count)
	{
		for (;;)
		{
			// the set of file descriptors for select
			fd_set fds;
			FD_ZERO(&fds);
			FD_SET(_fileDescriptor, &fds);

			// timeout
			struct timeval tv;
			tv.tv_sec = 2;
			tv.tv_usec = 0;

			// block until data is available
			int r = select(_fileDescriptor + 1, &fds, NULL, NULL, &tv);

			if (-1 == r)
			{
				if (EINTR == errno)
					continue;
				throw_errno_exception("select");
			}

			if (0 == r)
			{
				throw_exception("select timeout");
			}

			if (read_frame())
			{
				break;
			}

			/* EAGAIN - continue select loop. */
		}
	}
}

void V4L2Grabber::stop()
{
	stop_capturing();
}

void V4L2Grabber::open_device()
{
	struct stat st;

	if (-1 == stat(_deviceName.c_str(), &st))
	{
		std::ostringstream oss;
		oss << "Cannot identify '" << _deviceName << "'";
		throw_errno_exception(oss.str());
	}

	if (!S_ISCHR(st.st_mode))
	{
		std::ostringstream oss;
		oss << "'" << _deviceName << "' is no device";
		throw_exception(oss.str());
	}

	_fileDescriptor = open(_deviceName.c_str(), O_RDWR /* required */ | O_NONBLOCK, 0);

	if (-1 == _fileDescriptor)
	{
		std::ostringstream oss;
		oss << "Cannot open '" << _deviceName << "'";
		throw_errno_exception(oss.str());
	}
}

void V4L2Grabber::close_device()
{
	if (-1 == close(_fileDescriptor))
		throw_errno_exception("close");

	_fileDescriptor = -1;
}

void V4L2Grabber::init_read(unsigned int buffer_size)
{
	_buffers.resize(1);

	_buffers[0].length = buffer_size;
	_buffers[0].start = malloc(buffer_size);

	if (!_buffers[0].start) {
		throw_exception("Out of memory");
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
			std::ostringstream oss;
			oss << "'" << _deviceName << "' does not support memory mapping";
			throw_exception(oss.str());
		} else {
			throw_errno_exception("VIDIOC_REQBUFS");
		}
	}

	if (req.count < 2) {
		std::ostringstream oss;
		oss << "Insufficient buffer memory on " << _deviceName;
		throw_exception(oss.str());
	}

	_buffers.resize(req.count);

	for (size_t n_buffers = 0; n_buffers < req.count; ++n_buffers) {
		struct v4l2_buffer buf;

		CLEAR(buf);

		buf.type        = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buf.memory      = V4L2_MEMORY_MMAP;
		buf.index       = n_buffers;

		if (-1 == xioctl(VIDIOC_QUERYBUF, &buf))
			throw_errno_exception("VIDIOC_QUERYBUF");

		_buffers[n_buffers].length = buf.length;
		_buffers[n_buffers].start =
				mmap(NULL /* start anywhere */,
					 buf.length,
					 PROT_READ | PROT_WRITE /* required */,
					 MAP_SHARED /* recommended */,
					 _fileDescriptor, buf.m.offset);

		if (MAP_FAILED == _buffers[n_buffers].start)
			throw_errno_exception("mmap");
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
			std::ostringstream oss;
			oss << "'" << _deviceName << "' does not support user pointer";
			throw_exception(oss.str());
		} else {
			throw_errno_exception("VIDIOC_REQBUFS");
		}
	}

	_buffers.resize(4);

	for (size_t n_buffers = 0; n_buffers < 4; ++n_buffers) {
		_buffers[n_buffers].length = buffer_size;
		_buffers[n_buffers].start = malloc(buffer_size);

		if (!_buffers[n_buffers].start) {
			throw_exception("Out of memory");
		}
	}
}

void V4L2Grabber::init_device(VideoStandard videoStandard, int input)
{
	struct v4l2_capability cap;
	if (-1 == xioctl(VIDIOC_QUERYCAP, &cap))
	{
		if (EINVAL == errno) {
			std::ostringstream oss;
			oss << "'" << _deviceName << "' is no V4L2 device";
			throw_exception(oss.str());
		} else {
			throw_errno_exception("VIDIOC_QUERYCAP");
		}
	}

	if (!(cap.capabilities & V4L2_CAP_VIDEO_CAPTURE))
	{
		std::ostringstream oss;
		oss << "'" << _deviceName << "' is no video capture device";
		throw_exception(oss.str());
	}

	switch (_ioMethod) {
	case IO_METHOD_READ:
		if (!(cap.capabilities & V4L2_CAP_READWRITE))
		{
			std::ostringstream oss;
			oss << "'" << _deviceName << "' does not support read i/o";
			throw_exception(oss.str());
		}
		break;

	case IO_METHOD_MMAP:
	case IO_METHOD_USERPTR:
		if (!(cap.capabilities & V4L2_CAP_STREAMING))
		{
			std::ostringstream oss;
			oss << "'" << _deviceName << "' does not support streaming i/o";
			throw_exception(oss.str());
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
		}
	}

	// set the video standard if needed
	switch (videoStandard)
	{
	case PAL:
	{
		v4l2_std_id std_id = V4L2_STD_PAL;
		if (-1 == xioctl(VIDIOC_S_STD, &std_id))
		{
			throw_errno_exception("VIDIOC_S_STD");
		}
	}
		break;
	case NTSC:
	{
		v4l2_std_id std_id = V4L2_STD_NTSC;
		if (-1 == xioctl(VIDIOC_S_STD, &std_id))
		{
			throw_errno_exception("VIDIOC_S_STD");
		}
	}
		break;
	case NO_CHANGE:
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
	}

	// check pixel format
	switch (fmt.fmt.pix.pixelformat)
	{
	case V4L2_PIX_FMT_UYVY:
	case V4L2_PIX_FMT_YUYV:
		_pixelFormat = fmt.fmt.pix.pixelformat;
		break;
	default:
		throw_exception("Only pixel formats UYVY and YUYV are supported");
	}

	if (_width > 0 || _height > 0)
	{
		if (_width > 0)
		{
			fmt.fmt.pix.width = _width;
		}

		if (fmt.fmt.pix.height > 0)
		{
			fmt.fmt.pix.height = _height;
		}

		// set the settings
		if (-1 == xioctl(VIDIOC_S_FMT, &fmt))
		{
			throw_errno_exception("VIDIOC_S_FMT");
		}

		// get the format settings again
		// (the size may not have been accepted without an error)
		if (-1 == xioctl(VIDIOC_G_FMT, &fmt))
		{
			throw_errno_exception("VIDIOC_G_FMT");
		}
	}

	// store width & height
	_width = fmt.fmt.pix.width;
	_height = fmt.fmt.pix.height;

	// print the eventually used width and height
	std::cout << "V4L2 width=" << _width << " height=" << _height << std::endl;

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
				throw_errno_exception("munmap");
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
				throw_errno_exception("VIDIOC_QBUF");
		}
		v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(VIDIOC_STREAMON, &type))
			throw_errno_exception("VIDIOC_STREAMON");
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
				throw_errno_exception("VIDIOC_QBUF");
		}
		v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		if (-1 == xioctl(VIDIOC_STREAMON, &type))
			throw_errno_exception("VIDIOC_STREAMON");
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
		if (-1 == xioctl(VIDIOC_STREAMOFF, &type))
			throw_errno_exception("VIDIOC_STREAMOFF");
		break;
	}
}

int V4L2Grabber::read_frame()
{
	bool rc = false;

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
			}
		}

		assert(buf.index < _buffers.size());

		rc = process_image(_buffers[buf.index].start, buf.bytesused);

		if (-1 == xioctl(VIDIOC_QBUF, &buf))
		{
			throw_errno_exception("VIDIOC_QBUF");
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
		}
		break;
	}

	return rc ? 1 : 0;
}

bool V4L2Grabber::process_image(const void *p, int size)
{
	if (++_currentFrame >= _frameDecimation)
	{
		// We do want a new frame...

		if (size != 2*_width*_height)
		{
			std::cout << "Frame too small: " << size << " != " << (2*_width*_height) << std::endl;
		}
		else
		{
			process_image(reinterpret_cast<const uint8_t *>(p));
			_currentFrame = 0; // restart counting
			return true;
		}
	}

	return false;
}

void V4L2Grabber::process_image(const uint8_t * data)
{
	int width = (_width - 2 * _cropWidth + _pixelDecimation/2) / _pixelDecimation;
	int height = (_height - 2 * _cropHeight + _pixelDecimation/2) / _pixelDecimation;

	Image<ColorRgb> image(width, height);

	for (int ySource = _cropHeight + _pixelDecimation/2, yDest = 0; ySource < _height - _cropHeight; ySource += _pixelDecimation, ++yDest)
	{
		for (int xSource = _cropWidth + _pixelDecimation/2, xDest = 0; xSource < _width - _cropWidth; xSource += _pixelDecimation, ++xDest)
		{
			int index = (_width * ySource + xSource) * 2;
			uint8_t y = 0;
			uint8_t u = 0;
			uint8_t v = 0;

			switch (_pixelFormat)
			{
			case V4L2_PIX_FMT_UYVY:
				y = data[index+1];
				u = (xSource%2 == 0) ? data[index  ] : data[index-2];
				v = (xSource%2 == 0) ? data[index+2] : data[index  ];
				break;
			case V4L2_PIX_FMT_YUYV:
				y = data[index];
				u = (xSource%2 == 0) ? data[index+1] : data[index-1];
				v = (xSource%2 == 0) ? data[index+3] : data[index+1];
				break;
			}

			ColorRgb & rgb = image(xDest, yDest);
			yuv2rgb(y, u, v, rgb.red, rgb.green, rgb.blue);
		}
	}

	if (_callback != nullptr)
	{
		(*_callback)(_callbackArg, image);
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

void V4L2Grabber::throw_exception(const std::string & error)
{
	std::ostringstream oss;
	oss << error << " error";
	throw std::runtime_error(oss.str());
}

void V4L2Grabber::throw_errno_exception(const std::string & error)
{
	std::ostringstream oss;
	oss << error << " error " << errno << ", " << strerror(errno);
	throw std::runtime_error(oss.str());
}
