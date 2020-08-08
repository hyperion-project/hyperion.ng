#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/V4L2Grabber.h>

class V4L2Wrapper : public GrabberWrapper
{
	Q_OBJECT

public:
	V4L2Wrapper(const QString & device,
			const unsigned grabWidth,
			const unsigned grabHeight,
			const unsigned fps,
			const unsigned input,
			VideoStandard videoStandard,
			PixelFormat pixelFormat,
			int pixelDecimation );
	~V4L2Wrapper() override;

	bool getSignalDetectionEnable() const;
	bool getCecDetectionEnable() const;

public slots:
	bool start() override;
	void stop() override;

	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold);
	void setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom) override;
	void setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax);
	void setSignalDetectionEnable(bool enable);
	void setCecDetectionEnable(bool enable);
	void setDeviceVideoStandard(const QString& device, VideoStandard videoStandard);
	void handleCecEvent(CECEvent event);
	void handleSettingsUpdate(settings::type type, const QJsonDocument& config) override;

private slots:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

	void action() override;

private:
	/// The V4L2 grabber
	V4L2Grabber _grabber;
};
