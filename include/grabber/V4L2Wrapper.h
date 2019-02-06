#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/V4L2Grabber.h>

class V4L2Wrapper : public GrabberWrapper
{
	Q_OBJECT

public:
	V4L2Wrapper(const QString & device,
			VideoStandard videoStandard,
			PixelFormat pixelFormat,
			int pixelDecimation );
	virtual ~V4L2Wrapper() {};

	bool getSignalDetectionEnable();

public slots:
	bool start();
	void stop();

	void setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold);
	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom);
	void setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax);
	void setSignalDetectionEnable(bool enable);

private slots:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

	virtual void action();

private:
	/// The V4L2 grabber
	V4L2Grabber _grabber;
};
