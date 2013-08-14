#pragma once

// QT includes
#include <QObject>
#include <QTimer>

// Utils includes
#include <utils/RgbColor.h>

// Forward class declaration
class DispmanxFrameGrabber;
class Hyperion;
class ImageProcessor;

///
/// The DispmanxWrapper uses an instance of the DispmanxFrameGrabber to obtain RgbImage's from the
/// displayed content. This RgbImage is processed to a RgbColor for each led and commmited to the
/// attached Hyperion.
///
class DispmanxWrapper: public QObject
{
	Q_OBJECT
public:
	DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, Hyperion * hyperion);

	virtual ~DispmanxWrapper();

signals:
	void ledValues(const unsigned priority, const std::vector<RgbColor> ledColors, const unsigned timeout_ms);

public slots:
	void start();

	void action();

	void stop();

private:
	const int _updateInterval_ms;
	const int _timeout_ms;


	QTimer _timer;

	DispmanxFrameGrabber * _frameGrabber;
	ImageProcessor * _processor;

	std::vector<RgbColor> _ledColors;

	Hyperion * _hyperion;
};

