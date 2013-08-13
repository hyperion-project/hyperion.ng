#pragma once

// QT includes
#include <QObject>
#include <QTimer>

// Forward class declaration
class ImageProcessor;
class DispmanxFrameGrabber;

class DispmanxWrapper: public QObject
{
	Q_OBJECT
public:
	DispmanxWrapper();

	virtual ~DispmanxWrapper();

public slots:
	void start();

	void action();

	void stop();

private:
	QTimer _timer;

	DispmanxFrameGrabber* _frameGrabber;
	ImageProcessor* _processor;
};

