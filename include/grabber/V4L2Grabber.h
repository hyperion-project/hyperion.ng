#pragma once

// stl includes
#include <vector>
#include <map>

// Qt includes
#include <QObject>
#include <QSocketNotifier>
#include <QRectF>

// util includes
#include <utils/PixelFormat.h>
#include <hyperion/Grabber.h>
#include <grabber/VideoStandard.h>

/// Capture class for V4L2 devices
///
/// @see http://linuxtv.org/downloads/v4l-dvb-apis/capture-example.html
class V4L2Grabber : public Grabber
{
	Q_OBJECT

public:
	V4L2Grabber(const QString & device,
			int input,
			VideoStandard videoStandard,
			PixelFormat pixelFormat,
			int pixelDecimation
	);
	virtual ~V4L2Grabber();

	QRectF getSignalDetectionOffset();
	bool getSignalDetectionEnabled();

	int grabFrame(Image<ColorRgb> &);

	///
	/// @brief  overwrite Grabber.h implementation, as v4l doesn't use width/height
	///
	virtual void setWidthHeight(){};

	///
	/// @brief  set new PixelDecimation value to ImageResampler
	/// @param  pixelDecimation  The new pixelDecimation value
	///
	virtual void setPixelDecimation(int pixelDecimation);

	///
	/// @brief  overwrite Grabber.h implementation
	///
	virtual void setSignalThreshold(
					double redSignalThreshold,
					double greenSignalThreshold,
					double blueSignalThreshold,
					int noSignalCounterThreshold = 50);

	///
	/// @brief  overwrite Grabber.h implementation
	///
	virtual void setSignalDetectionOffset(
					double verticalMin,
					double horizontalMin,
					double verticalMax,
					double horizontalMax);
	///
	/// @brief  overwrite Grabber.h implementation
	///
	virtual void setSignalDetectionEnable(bool enable);

	///
	/// @brief overwrite Grabber.h implementation
	/// 
	virtual void setInputVideoStandard(int input, VideoStandard videoStandard);

public slots:

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

	void throw_exception(const QString &error);

	void throw_errno_exception(const QString &error);

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
	QString _deviceName;
	std::map<QString,QString> _v4lDevices;
	int                 _input;
	VideoStandard       _videoStandard;
	io_method           _ioMethod;
	int                 _fileDescriptor;
	std::vector<buffer> _buffers;

	PixelFormat _pixelFormat;
	int         _pixelDecimation;
	int         _lineLength;
	int         _frameByteSize;

	// signal detection
	int      _noSignalCounterThreshold;
	ColorRgb _noSignalThresholdColor;
	bool     _signalDetectionEnabled;
	bool     _noSignalDetected;
	int      _noSignalCounter;
	double   _x_frac_min;
	double   _y_frac_min;
	double   _x_frac_max;
	double   _y_frac_max;

	QSocketNotifier * _streamNotifier;

	bool _initialized;
	bool _deviceAutoDiscoverEnabled;
};
