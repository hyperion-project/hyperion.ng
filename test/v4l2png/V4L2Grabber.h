#pragma once

// stl includes
#include <string>
#include <vector>

/// Capture class for V4L2 devices
///
/// @see http://linuxtv.org/downloads/v4l-dvb-apis/capture-example.html
class V4L2Grabber
{
public:
	V4L2Grabber();
	virtual ~V4L2Grabber();

	void start();

private:
	void open_device();

	void close_device();

	void init_read(unsigned int buffer_size);

	void init_mmap();

	void init_userp(unsigned int buffer_size);

	void init_device();

	void uninit_device();

	void start_capturing();

	void stop_capturing();

	int read_frame();

	void process_image(const void *p, int size);

	int xioctl(int request, void *arg);

	void errno_exit(const char *s);

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
};
