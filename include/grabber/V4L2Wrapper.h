#pragma once

// Qt includes
#include <QTimer>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessor.h>

// Grabber includes
#include <grabber/V4L2Grabber.h>

class V4L2Wrapper : public QObject
{
	Q_OBJECT

public:
	V4L2Wrapper(const std::string & device,
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
			Hyperion * hyperion,
			int hyperionPriority);
	virtual ~V4L2Wrapper();

public slots:
	void start();

	void stop();

	void setCropping(int cropLeft,
					 int cropRight,
					 int cropTop,
					 int cropBottom);

	void set3D(VideoMode mode);

signals:
	void emitColors(int priority, const std::vector<ColorRgb> &ledColors, const int timeout_ms);

private slots:
	void newFrame(const Image<ColorRgb> & image);

	void checkSources();

private:
	/// The timeout of the led colors [ms]
	const int _timeout_ms;

	/// The priority of the led colors
	const int _priority;

	/// The V4L2 grabber
	V4L2Grabber _grabber;

	/// The processor for transforming images to led colors
	ImageProcessor * _processor;

	/// The Hyperion instance
	Hyperion * _hyperion;

	/// The list with computed led colors
	std::vector<ColorRgb> _ledColors;

	/// Timer which tests if a higher priority source is active
	QTimer _timer;
};
