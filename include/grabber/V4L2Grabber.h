#pragma once

// stl includes
#include <string>
#include <vector>

// util includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>

/// Capture class for V4L2 devices
///
/// @see http://linuxtv.org/downloads/v4l-dvb-apis/capture-example.html
class V4L2Grabber
{
public:
	typedef void (*ImageCallback)(void * arg, const Image<ColorRgb> & image);

	enum VideoStandard {
		PAL, NTSC, NO_CHANGE
	};

	enum Mode3D {
		MODE_NONE, MODE_3DSBS, MODE_3DTAB
	};

public:
	V4L2Grabber(
			const std::string & device,
			int input,
			VideoStandard videoStandard,
			int width,
			int height,
			int frameDecimation,
			int horizontalPixelDecimation,
			int verticalPixelDecimation);
	virtual ~V4L2Grabber();

	void setCropping(int cropLeft,
					 int cropRight,
					 int cropTop,
					 int cropBottom);

	void set3D(Mode3D mode);

	void setCallback(ImageCallback callback, void * arg);

	void start();

	void capture(int frameCount = -1);

	void stop();

private:
	void open_device();

	void close_device();

	void init_read(unsigned int buffer_size);

	void init_mmap();

	void init_userp(unsigned int buffer_size);

	void init_device(VideoStandard videoStandard, int input);

	void uninit_device();

	void start_capturing();

	void stop_capturing();

	int read_frame();

	bool process_image(const void *p, int size);

	void process_image(const uint8_t *p);

	int xioctl(int request, void *arg);

	void throw_exception(const std::string &error);

	void throw_errno_exception(const std::string &error);

private:
	enum io_method {
			IO_METHOD_READ,
			IO_METHOD_MMAP,
			IO_METHOD_USERPTR
	};

	struct buffer {
			void   *start;
			size_t  length;
	};

private:
	const std::string _deviceName;
	const io_method _ioMethod;
	int _fileDescriptor;
	std::vector<buffer> _buffers;

	uint32_t _pixelFormat;
	int _width;
	int _height;
	int _cropLeft;
	int _cropRight;
	int _cropTop;
	int _cropBottom;
	int _frameDecimation;
	int _horizontalPixelDecimation;
	int _verticalPixelDecimation;

	Mode3D _mode3D;

	int _currentFrame;

	ImageCallback _callback;
	void * _callbackArg;
};
