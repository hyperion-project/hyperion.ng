#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/MFGrabber.h>

class MFWrapper : public GrabberWrapper
{
	Q_OBJECT

public:
	MFWrapper(const QString & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned fps, int pixelDecimation, QString flipMode);
	~MFWrapper() override;

	bool getSignalDetectionEnable() const;
	bool getCecDetectionEnable() const;

public slots:
	bool start() override;
	void stop() override;

	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold, int noSignalCounterThreshold);
	void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom) override;
	void setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax);
	void setSignalDetectionEnable(bool enable);
	void setCecDetectionEnable(bool enable);
	bool setDevice(const QString& device);
	void setFpsSoftwareDecimation(int decimation);
	bool setEncoding(QString enc);
	bool setBrightnessContrastSaturationHue(int brightness, int contrast, int saturation, int hue);
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config) override;

private slots:
	void newFrame(const Image<ColorRgb> & image);

	void action() override;

private:
	/// The Media Foundation grabber
	MFGrabber _grabber;
};
