#pragma once

// stl includes
#include <vector>
#include <map>

// Qt includes
#include <QObject>
#include <QSocketNotifier>
#include <QRectF>
#include <QMap>
#include <QMultiMap>

// util includes
#include <utils/PixelFormat.h>
#include <hyperion/Grabber.h>
#include <hyperion/GrabberWrapper.h>
#include <utils/VideoStandard.h>
#include <utils/Components.h>

// decoder thread includes
#include <grabber/EncoderThread.h>

// Determine the cmake options
#include <HyperionConfig.h>

#if defined(ENABLE_CEC)
	#include <cec/CECEvent.h>
#endif

///
/// Capture class for V4L2 devices
///

class V4L2Grabber : public Grabber
{
	Q_OBJECT

public:
	struct DeviceProperties
	{
		QString name = QString();
		struct InputProperties
		{
			QString inputName = QString();
			QList<VideoStandard> standards = QList<VideoStandard>();
			struct EncodingProperties
			{
				int width		= 0;
				int height		= 0;
				QList<int> framerates	= QList<int>();
			};
			QMultiMap<PixelFormat, EncodingProperties> encodingFormats = QMultiMap<PixelFormat, EncodingProperties>();
		};
		QMap<int, InputProperties> inputs = QMap<int, InputProperties>();
	};

	struct DeviceControls
	{
		QString property = QString();
		int minValue = 0;
		int maxValue = 0;
		int step = 0;
		int defaultValue = 0;
		int currentValue = 0;
	};

	V4L2Grabber();
	~V4L2Grabber() override;

	int grabFrame(Image<ColorRgb> &);
	void setDevice(const QString& devicePath, const QString& deviceName);
	bool setInput(int input) override;
	bool setWidthHeight(int width, int height) override;
	void setEncoding(QString enc);
	void setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue);
	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold = 50);
	void setSignalDetectionOffset( double verticalMin, double horizontalMin, double verticalMax, double horizontalMax);
	void setSignalDetectionEnable(bool enable);
	void setCecDetectionEnable(bool enable);
	bool reload(bool force = false);

	QRectF getSignalDetectionOffset() const { return QRectF(_x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max); } //used from hyperion-v4l2



	///
	/// @brief Discover available V4L2 USB devices (for configuration).
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	/// @return A JSON structure holding a list of USB devices found
	///
	QJsonArray discover(const QJsonObject& params);

public slots:
	bool prepare();
	bool start();
	void stop();
	void newThreadFrame(Image<ColorRgb> image);

#if defined(ENABLE_CEC)
	void handleCecEvent(CECEvent event);
#endif

signals:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

private slots:
	int read_frame();

private:
	bool init();
	void uninit();
	bool open_device();
	void close_device();
	void init_read(unsigned int buffer_size);
	void init_mmap();
	void init_userp(unsigned int buffer_size);
	void init_device(VideoStandard videoStandard);
	void uninit_device();
	void start_capturing();
	void stop_capturing();
	bool process_image(const void *p, int size);
	int xioctl(int request, void *arg);
	int xioctl(int fileDescriptor, int request, void *arg);

	void throw_exception(const QString & error)
	{
		Error(_log, "Throws error: %s", QSTRING_CSTR(error));
	}

	void throw_errno_exception(const QString & error)
	{
		Error(_log, "Throws error nr: %s", QSTRING_CSTR(QString(error + " error code " + QString::number(errno) + ", " + strerror(errno))));
	}

private:
	enum io_method
	{
			IO_METHOD_READ,
			IO_METHOD_MMAP,
			IO_METHOD_USERPTR
	};

	struct buffer
	{
			void   *start;
			size_t  length;
	};

private:
	QString _currentDevicePath, _currentDeviceName;
	EncoderThreadManager* _threadManager;
	QMap<QString, V4L2Grabber::DeviceProperties> _deviceProperties;
	QMap<QString, QList<DeviceControls>> _deviceControls;

	io_method           _ioMethod;
	int                 _fileDescriptor;
	std::vector<buffer> _buffers;

	PixelFormat _pixelFormat, _pixelFormatConfig;
	int         _lineLength;
	int         _frameByteSize;

	QAtomicInt _currentFrame;

	// signal detection
	int      _noSignalCounterThreshold;
	ColorRgb _noSignalThresholdColor;
	bool     _cecDetectionEnabled, _cecStandbyActivated, _signalDetectionEnabled, _noSignalDetected;
	int      _noSignalCounter;
	int		_brightness, _contrast, _saturation, _hue;
	double   _x_frac_min;
	double   _y_frac_min;
	double   _x_frac_max;
	double   _y_frac_max;

	QSocketNotifier *_streamNotifier;

	bool _initialized, _reload;

protected:
	void enumVideoCaptureDevices();
	void enumFrameIntervals(QList<int> &framerates, int fileDescriptor, int pixelformat, int width, int height);
};
