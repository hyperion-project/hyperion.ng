#pragma once

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/GrabberWrapper.h>

// Grabber includes
#include <grabber/V4L2Grabber.h>

class V4L2Wrapper : public GrabberWrapper
{
	Q_OBJECT

public:
	V4L2Wrapper(const QString & device,
			int input,
			VideoStandard videoStandard,
			PixelFormat pixelFormat,
			int width,
			int height,
			int frameDecimation,
			int pixelDecimation,
			double redSignalThreshold,
			double greenSignalThreshold,
			double blueSignalThreshold,
			const int priority);
	virtual ~V4L2Wrapper();

	bool getSignalDetectionEnable();

public slots:
	bool start();
	void stop();

	void setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom);
	void setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax);
	void set3D(VideoMode mode);
	void setSignalDetectionEnable(bool enable);

// signals:
// 	void emitColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms);

private slots:
	void newFrame(const Image<ColorRgb> & image);
	void readError(const char* err);

	virtual void action();
	void checkSources();

private:
	/// The timeout of the led colors [ms]
	const int _timeout_ms;

	/// The V4L2 grabber
	V4L2Grabber _grabber;

	/// The list with computed led colors
	std::vector<ColorRgb> _ledColors;
};
