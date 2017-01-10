#pragma once

// stl includes
#include <string>
#include <vector>
#include <map>

// Qt includes
#include <QObject>
#include <QSocketNotifier>
#include <QRectF>

// util includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/PixelFormat.h>
#include <utils/VideoMode.h>
#include <utils/ImageResampler.h>
#include <utils/Logger.h>

// grabber includes
#include <grabber/VideoStandard.h>

/// Capture class for V4L2 devices
///
/// @see http://linuxtv.org/downloads/v4l-dvb-apis/capture-example.html
class V4L2Grabber : public QObject
{
	Q_OBJECT

public:
	V4L2Grabber(const std::string & device,
			int input,
			VideoStandard videoStandard, PixelFormat pixelFormat,
			int width,
			int height,
			int frameDecimation,
			int horizontalPixelDecimation,
			int verticalPixelDecimation
	);
	virtual ~V4L2Grabber();

	QRectF getSignalDetectionOffset();

public slots:
	void setCropping(int cropLeft,
					 int cropRight,
					 int cropTop,
					 int cropBottom);

	void set3D(VideoMode mode);

	void setSignalThreshold(
					double redSignalThreshold,
					double greenSignalThreshold,
					double blueSignalThreshold,
					int noSignalCounterThreshold);

	void setSignalDetectionOffset(
					double verticalMin,
					double horizontalMin,
					double verticalMax,
					double horizontalMax);

	bool start();

	void stop();

signals:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

private slots:
	int read_frame();

private:
	void getV4Ldevices();
	
	bool init();
	void uninit();

	void open_device();

	void close_device();

	void init_read(unsigned int buffer_size);

	void init_mmap();

	void init_userp(unsigned int buffer_size);

	void init_device(VideoStandard videoStandard, int input);

	void uninit_device();

	void start_capturing();

	void stop_capturing();

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
	std::string _deviceName;
	std::map<std::string,std::string> _v4lDevices;
	int _input;
	VideoStandard _videoStandard;
	io_method _ioMethod;
	int _fileDescriptor;
	std::vector<buffer> _buffers;

	PixelFormat _pixelFormat;
	int _width;
	int _height;
	int _lineLength;
	int _frameByteSize;
	int _frameDecimation;
	int _noSignalCounterThreshold;

	ColorRgb _noSignalThresholdColor;

	int _currentFrame;
	int _noSignalCounter;

	QSocketNotifier * _streamNotifier;

	ImageResampler _imageResampler;
	
	Logger * _log;
	bool _initialized;
	bool _deviceAutoDiscoverEnabled;
	
	bool  _noSignalDetected;
	double _x_frac_min;
	double _y_frac_min;
	double _x_frac_max;
	double _y_frac_max;

};
