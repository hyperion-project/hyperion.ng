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

#include "grabber/V4L2Grabber.h"

#define CLEAR(x) memset(&(x), 0, sizeof(x))

V4L2Grabber::V4L2Grabber(const std::string & device,
        int input,
        VideoStandard videoStandard,
        PixelFormat pixelFormat,
        int width,
        int height,
        int frameDecimation,
        int horizontalPixelDecimation,
        int verticalPixelDecimation) :
    _deviceName(device),
    _ioMethod(IO_METHOD_MMAP),
    _fileDescriptor(-1),
    _buffers(),
    _pixelFormat(pixelFormat),
    _width(width),
    _height(height),
    _lineLength(-1),
    _frameByteSize(-1),
    _frameDecimation(std::max(1, frameDecimation)),
    _noSignalCounterThreshold(50),
    _noSignalThresholdColor(ColorRgb{0,0,0}),
    _currentFrame(0),
    _noSignalCounter(0),
    _streamNotifier(nullptr),
    _imageResampler()
{
    _imageResampler.setHorizontalPixelDecimation(std::max(1, horizontalPixelDecimation));
    _imageResampler.setVerticalPixelDecimation(std::max(1, verticalPixelDecimation));

    open_device();
    init_device(videoStandard, input);
}

V4L2Grabber::~V4L2Grabber()
{
    // stop if the grabber was not stopped
    stop();
    uninit_device();
    close_device();
}

void V4L2Grabber::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
    _imageResampler.setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void V4L2Grabber::set3D(VideoMode mode)
{
    _imageResampler.set3D(mode);
}

void V4L2Grabber::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold)
{
    _noSignalThresholdColor.red = uint8_t(255*redSignalThreshold);
    _noSignalThresholdColor.green = uint8_t(255*greenSignalThreshold);
    _noSignalThresholdColor.blue = uint8_t(255*blueSignalThreshold);
    _noSignalCounterThreshold = std::max(1, noSignalCounterThreshold);

    std::cout << "V4L2 grabber signal threshold set to: " << _noSignalThresholdColor << std::endl;
}

void V4L2Grabber::start()
{
    if (_streamNotifier != nullptr && !_streamNotifier->isEnabled())
    {
        _streamNotifier->setEnabled(true);
        start_capturing();
        std::cout << "V4L2 grabber started" << std::endl;
    }
}

void V4L2Grabber::stop()
{
    if (_streamNotifier != nullptr && _streamNotifier->isEnabled())
    {
        stop_capturing();
        _streamNotifier->setEnabled(false);
        std::cout << "V4L2 grabber stopped" << std::endl;
    }
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

    // create the notifier for when a new frame is available
    _streamNotifier = new QSocketNotifier(_fileDescriptor, QSocketNotifier::Read);
    _streamNotifier->setEnabled(false);
    connect(_streamNotifier, SIGNAL(activated(int)), this, SLOT(read_frame()));
}

void V4L2Grabber::close_device()
{
    if (-1 == close(_fileDescriptor))
        throw_errno_exception("close");

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
    case VIDEOSTANDARD_PAL:
    {
        v4l2_std_id std_id = V4L2_STD_PAL;
        if (-1 == xioctl(VIDIOC_S_STD, &std_id))
        {
            throw_errno_exception("VIDIOC_S_STD");
        }
    }
        break;
    case VIDEOSTANDARD_NTSC:
    {
        v4l2_std_id std_id = V4L2_STD_NTSC;
        if (-1 == xioctl(VIDIOC_S_STD, &std_id))
        {
            throw_errno_exception("VIDIOC_S_STD");
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

    // set the requested withd and height
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
    }

    // set the line length
    _lineLength = fmt.fmt.pix.bytesperline;

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

    // store width & height
    _width = fmt.fmt.pix.width;
    _height = fmt.fmt.pix.height;

    // print the eventually used width and height
    std::cout << "V4L2 width=" << _width << " height=" << _height << std::endl;

    // check pixel format and frame size
    switch (fmt.fmt.pix.pixelformat)
    {
    case V4L2_PIX_FMT_UYVY:
        _pixelFormat = PIXELFORMAT_UYVY;
        _frameByteSize = _width * _height * 2;
        std::cout << "V4L2 pixel format=UYVY" << std::endl;
        break;
    case V4L2_PIX_FMT_YUYV:
        _pixelFormat = PIXELFORMAT_YUYV;
        _frameByteSize = _width * _height * 2;
        std::cout << "V4L2 pixel format=YUYV" << std::endl;
        break;
    case V4L2_PIX_FMT_RGB32:
        _pixelFormat = PIXELFORMAT_RGB32;
        _frameByteSize = _width * _height * 4;
        std::cout << "V4L2 pixel format=RGB32" << std::endl;
        break;
    default:
        throw_exception("Only pixel formats UYVY, YUYV, and RGB32 are supported");
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

        if (size != _frameByteSize)
        {
            std::cout << "Frame too small: " << size << " != " << _frameByteSize << std::endl;
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
    Image<ColorRgb> image(0, 0);
    _imageResampler.processImage(data, _width, _height, _lineLength, _pixelFormat, image);

    // check signal (only in center of the resulting image, because some grabbers have noise values along the borders)
    bool noSignal = true;
    for (unsigned x = 0; noSignal && x < (image.width()>>1); ++x)
    {
        int xImage = (image.width()>>2) + x;

        for (unsigned y = 0; noSignal && y < (image.height()>>1); ++y)
        {
            int yImage = (image.height()>>2) + y;

            ColorRgb & rgb = image(xImage, yImage);
            noSignal &= rgb <= _noSignalThresholdColor;
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
            std::cout << "V4L2 Grabber: " << "Signal detected" << std::endl;
        }

        _noSignalCounter = 0;
    }

    if (_noSignalCounter < _noSignalCounterThreshold)
    {
        emit newFrame(image);
    }
    else if (_noSignalCounter == _noSignalCounterThreshold)
    {
        std::cout << "V4L2 Grabber: " << "Signal lost" << std::endl;
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
