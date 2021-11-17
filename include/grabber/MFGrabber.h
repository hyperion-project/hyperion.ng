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
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

// utils includes
#include <utils/PixelFormat.h>
#include <utils/Components.h>
#include <hyperion/Grabber.h>

// decoder thread includes
#include <grabber/EncoderThread.h>

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

	struct DeviceControls
	{
		QString property = QString();
		int minValue = 0;
		int maxValue = 0;
		int step = 0;
		int def = 0;
		int currentValue = 0;
	};

	MFGrabber();
	~MFGrabber() override;

	void receive_image(const void *frameImageBuffer, int size);
	void setDevice(const QString& device);
	bool setInput(int input) override;
	bool setWidthHeight(int width, int height) override;
	void setEncoding(QString enc);
	void setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue);
	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold);
	void setSignalDetectionOffset( double verticalMin, double horizontalMin, double verticalMax, double horizontalMax);
	void setSignalDetectionEnable(bool enable);
	bool reload(bool force = false);

	///
	/// @brief Discover available Media Foundation USB devices (for configuration).
	/// @param[in] params Parameters used to overwrite discovery default behaviour
	/// @return A JSON structure holding a list of USB devices found
	///
	QJsonArray discover(const QJsonObject& params);

public slots:
	bool prepare();
	bool start();
	void stop();
	void newThreadFrame(Image<ColorRgb> image);

signals:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

private:
	bool init();
	void uninit();
	HRESULT init_device(QString device, DeviceProperties props);
	void enumVideoCaptureDevices();
	void start_capturing();
	void process_image(const void *frameImageBuffer, int size);

	QString										_currentDeviceName,
												_newDeviceName;
	QMap<QString, QList<DeviceProperties>>		_deviceProperties;
	QMap<QString, QList<DeviceControls>>		_deviceControls;
	HRESULT										_hr;
	IMFSourceReader*							_sourceReader;
	SourceReaderCB*								_sourceReaderCB;
	EncoderThreadManager*						_threadManager;
	PixelFormat									_pixelFormat,
												_pixelFormatConfig;
	int											_lineLength,
												_frameByteSize,
												_noSignalCounterThreshold,
												_noSignalCounter,
												_brightness,
												_contrast,
												_saturation,
												_hue;
	QAtomicInt									_currentFrame;
	ColorRgb									_noSignalThresholdColor;
	bool										_signalDetectionEnabled,
												_noSignalDetected,
												_initialized,
												_reload;
	double										_x_frac_min,
												_y_frac_min,
												_x_frac_max,
												_y_frac_max;
};
