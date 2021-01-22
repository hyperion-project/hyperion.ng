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
	struct DevicePropertiesItem
	{
		int         x, y,fps,fps_a,fps_b;
		PixelFormat pf;
		GUID        guid;
	};

	struct DeviceProperties
	{
		QString name						= QString();
		QMultiMap<QString, int>	inputs		= QMultiMap<QString, int>();
		QStringList displayResolutions		= QStringList();
		QStringList framerates				= QStringList();
		QList<DevicePropertiesItem> valid	= QList<DevicePropertiesItem>();
	};

	MFGrabber(const QString & device, const unsigned width, const unsigned height, const unsigned fps, const unsigned input, int pixelDecimation);
	~MFGrabber() override;

	void receive_image(const void *frameImageBuffer, int size);
	QRectF getSignalDetectionOffset() const { return QRectF(_x_frac_min, _y_frac_min, _x_frac_max, _y_frac_max); }
	bool getSignalDetectionEnabled() const { return _signalDetectionEnabled; }
	bool getCecDetectionEnabled() const { return _cecDetectionEnabled; }
	QStringList getV4L2devices() const override;
	QString getV4L2deviceName(const QString& devicePath) const override { return devicePath; }
	QMultiMap<QString, int> getV4L2deviceInputs(const QString& devicePath) const override { return _deviceProperties.value(devicePath).inputs; }
	QStringList getResolutions(const QString& devicePath) const override { return _deviceProperties.value(devicePath).displayResolutions; }
	QStringList getFramerates(const QString& devicePath) const override { return _deviceProperties.value(devicePath).framerates; }
	QStringList getV4L2EncodingFormats(const QString& devicePath) const override;
	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold) override;
	void setSignalDetectionOffset( double verticalMin, double horizontalMin, double verticalMax, double horizontalMax) override;
	void setSignalDetectionEnable(bool enable) override;
	void setPixelDecimation(int pixelDecimation) override;
	void setCecDetectionEnable(bool enable) override;
	void setDeviceVideoStandard(QString device, VideoStandard videoStandard) override;
	bool setInput(int input) override;
	bool setWidthHeight(int width, int height) override;
	bool setFramerate(int fps) override;
	void setFpsSoftwareDecimation(int decimation);
	void setEncoding(QString enc);
	void setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue);

public slots:
	bool start();
	void stop();
	void newThreadFrame(unsigned int _workerIndex, const Image<ColorRgb>& image,unsigned int sourceCount);

signals:
	void newFrame(const Image<ColorRgb> & image);

private:
	bool init();
	void uninit();
	HRESULT init_device(QString device, DevicePropertiesItem props);
	void uninit_device();
	void enumVideoCaptureDevices();
	void start_capturing();
	void process_image(const void *frameImageBuffer, int size);
	void checkSignalDetectionEnabled(Image<ColorRgb> image);

	QString										_deviceName;
	QMap<QString, MFGrabber::DeviceProperties>	_deviceProperties;
	HRESULT										_hr;
	SourceReaderCB*								_sourceReaderCB;
	PixelFormat									_pixelFormat;
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
