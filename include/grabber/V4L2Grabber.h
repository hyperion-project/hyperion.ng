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
#include <grabber/VideoStandard.h>
#include <utils/Components.h>
#include <cec/CECEvent.h>

// general JPEG decoder includes
#ifdef HAVE_JPEG_DECODER
	#include <QImage>
	#include <QColor>
#endif

// System JPEG decoder
#ifdef HAVE_JPEG
	#include <jpeglib.h>
	#include <csetjmp>
#endif

// TurboJPEG decoder
#ifdef HAVE_TURBO_JPEG
	#include <turbojpeg.h>
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
			struct EncodingProperties
			{
				unsigned int width		= 0;
				unsigned int height		= 0;
				QList<int> framerates	= QList<int>();
			};
			QMultiMap<PixelFormat, EncodingProperties> encodingFormats = QMultiMap<PixelFormat, EncodingProperties>();
		};
		QMap<int, InputProperties> inputs = QMap<int, InputProperties>();
	};

	V4L2Grabber(const QString & device, const unsigned width, const unsigned height, const unsigned fps, const unsigned input, VideoStandard videoStandard, PixelFormat pixelFormat, int pixelDecimation);
	~V4L2Grabber() override;

	QRectF getSignalDetectionOffset() const
	{
		return QRectF(_x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max);
	}

	bool getSignalDetectionEnabled() const { return _signalDetectionEnabled; }
	bool getCecDetectionEnabled() const { return _cecDetectionEnabled; }

	int grabFrame(Image<ColorRgb> &);

	///
	/// @brief  set new PixelDecimation value to ImageResampler
	/// @param  pixelDecimation  The new pixelDecimation value
	///
	void setPixelDecimation(int pixelDecimation) override;

	///
	/// @brief  overwrite Grabber.h implementation
	///
	void setSignalThreshold(
					double redSignalThreshold,
					double greenSignalThreshold,
					double blueSignalThreshold,
					int noSignalCounterThreshold = 50) override;

	///
	/// @brief  overwrite Grabber.h implementation
	///
	void setSignalDetectionOffset(
					double verticalMin,
					double horizontalMin,
					double verticalMax,
					double horizontalMax) override;
	///
	/// @brief  overwrite Grabber.h implementation
	///
	void setSignalDetectionEnable(bool enable) override;

	///
	/// @brief  overwrite Grabber.h implementation
	///
	void setCecDetectionEnable(bool enable) override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	void setDeviceVideoStandard(QString device, VideoStandard videoStandard) override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	bool setInput(int input) override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	bool setWidthHeight(int width, int height) override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	bool setFramerate(int fps) override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	QStringList getDevices() const override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	QString getDeviceName(const QString& devicePath) const override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	QMultiMap<QString, int> getDeviceInputs(const QString& devicePath) const override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	QStringList getAvailableEncodingFormats(const QString& devicePath, const int& deviceInput) const override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	QMultiMap<int, int> getAvailableDeviceResolutions(const QString& devicePath, const int& deviceInput, const PixelFormat& encFormat) const override;

	///
	/// @brief overwrite Grabber.h implementation
	///
	QStringList getAvailableDeviceFramerates(const QString& devicePath, const int& deviceInput, const PixelFormat& encFormat, const unsigned width, const unsigned height) const override;


public slots:

	bool start();

	void stop();

	void handleCecEvent(CECEvent event);

signals:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

private slots:
	int read_frame();

private:
	void getV4Ldevices();

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

	void process_image(const uint8_t *p, int size);

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

#ifdef HAVE_JPEG
	struct errorManager
	{
		jpeg_error_mgr pub;
		jmp_buf setjmp_buffer;
	};

	static void errorHandler(j_common_ptr cInfo)
	{
		errorManager* mgr = reinterpret_cast<errorManager*>(cInfo->err);
		longjmp(mgr->setjmp_buffer, 1);
	}

	static void outputHandler(j_common_ptr cInfo)
	{
		// Suppress fprintf warnings.
	}

	jpeg_decompress_struct* _decompress;
	errorManager* _error;
#endif

#ifdef HAVE_TURBO_JPEG
	tjhandle _decompress = nullptr;
	int _subsamp;
#endif

private:
	QString _deviceName;
	std::map<QString, QString> _v4lDevices;
	QMap<QString, V4L2Grabber::DeviceProperties> _deviceProperties;

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
	bool     _cecDetectionEnabled;
	bool     _cecStandbyActivated;
	bool     _noSignalDetected;
	int      _noSignalCounter;
	double   _x_frac_min;
	double   _y_frac_min;
	double   _x_frac_max;
	double   _y_frac_max;

	QSocketNotifier *_streamNotifier;

	bool _initialized;
	bool _deviceAutoDiscoverEnabled;

protected:
	void enumFrameIntervals(QList<int> &framerates, int fileDescriptor, int pixelformat, int width, int height);
};
