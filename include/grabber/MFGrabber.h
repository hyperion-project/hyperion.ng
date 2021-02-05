#pragma once

// Windows include
#include <Windows.h>

// COM includes
#include <Guiddef.h>

// Qt includes
#include <QObject>
#include <QRectF>
#include <QMap>
#include <QMultiMap>

// utils includes
#include <utils/PixelFormat.h>
#include <utils/Components.h>
#include <hyperion/Grabber.h>

// decoder thread includes
#include <grabber/MFThread.h>

// TurboJPEG decoder
#ifdef HAVE_TURBO_JPEG
	#include <turbojpeg.h>
#endif

/// Forward class declaration
class SourceReaderCB;
/// Forward struct declaration
struct IMFSourceReader;

///
/// Media Foundation capture class
///

class MFGrabber : public Grabber
{
	Q_OBJECT
	friend class SourceReaderCB;

public:
	struct DeviceProperties
	{
		QString symlink = QString();
		int	width		= 0;
		int	height		= 0;
		int	fps			= 0;
		int numerator	= 0;
		int denominator = 0;
		PixelFormat pf	= PixelFormat::NO_CHANGE;
		GUID guid		= GUID_NULL;
	};

	MFGrabber(const QString & device, const unsigned width, const unsigned height, const unsigned fps, int pixelDecimation, QString flipMode);
	~MFGrabber() override;

	void receive_image(const void *frameImageBuffer, int size);
	QRectF getSignalDetectionOffset() const { return QRectF(_x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max); }
	bool getSignalDetectionEnabled() const { return _signalDetectionEnabled; }
	bool getCecDetectionEnabled() const { return _cecDetectionEnabled; }
	QStringList getDevices() const override;
	QString getDeviceName(const QString& devicePath) const override { return devicePath; }
	QMultiMap<QString, int> getDeviceInputs(const QString& devicePath) const override { return {{ devicePath, 0}}; }
	QStringList getAvailableEncodingFormats(const QString& devicePath, const int& /*deviceInput*/) const override;
	QStringList getAvailableDeviceResolutions(const QString& devicePath, const int& /*deviceInput*/, const PixelFormat& encFormat) const override;
	QStringList getAvailableDeviceFramerates(const QString& devicePath, const int& /*deviceInput*/, const PixelFormat& encFormat, const unsigned width, const unsigned height) const override;
	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold) override;
	void setSignalDetectionOffset( double verticalMin, double horizontalMin, double verticalMax, double horizontalMax) override;
	void setSignalDetectionEnable(bool enable) override;
	void setPixelDecimation(int pixelDecimation) override;
	void setCecDetectionEnable(bool enable) override;
	bool setDevice(QString device) override;
	bool setWidthHeight(int width, int height) override;
	bool setFramerate(int fps) override;
	void setFpsSoftwareDecimation(int decimation);
	bool setEncoding(QString enc);
	void setFlipMode(QString flipMode);
	bool setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue);

	void reloadGrabber();

public slots:
	bool start();
	void stop();
	void newThreadFrame(unsigned int _workerIndex, const Image<ColorRgb>& image,unsigned int sourceCount);

signals:
	void newFrame(const Image<ColorRgb> & image);

private:
	bool init();
	void uninit();
	HRESULT init_device(QString device, DeviceProperties props);
	void uninit_device();
	void enumVideoCaptureDevices();
	void start_capturing();
	void process_image(const void *frameImageBuffer, int size);
	void checkSignalDetectionEnabled(Image<ColorRgb> image);

	QString										_currentDeviceName, _newDeviceName;
	QMap<QString, QList<DeviceProperties>>		_deviceProperties;
	HRESULT										_hr;
	SourceReaderCB*								_sourceReaderCB;
	PixelFormat									_pixelFormat, _pixelFormatConfig;
	int											_pixelDecimation,
												_lineLength,
												_frameByteSize,
												_noSignalCounterThreshold,
												_noSignalCounter,
												_fpsSoftwareDecimation,
												_brightness,
												_contrast,
												_saturation,
												_hue;
	volatile unsigned int						_currentFrame;
	ColorRgb									_noSignalThresholdColor;
	bool										_signalDetectionEnabled,
												_cecDetectionEnabled,
												_noSignalDetected,
												_initialized;
	double										_x_frac_min,
												_y_frac_min,
												_x_frac_max,
												_y_frac_max;
	MFThreadManager								_threadManager;
	IMFSourceReader*							_sourceReader;

#ifdef HAVE_TURBO_JPEG
	int											_subsamp;
#endif
};
