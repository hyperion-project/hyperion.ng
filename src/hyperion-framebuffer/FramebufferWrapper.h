#pragma once

// QT includes
#include <QTimer>

// Hyperion-Dispmanx includes
#include <grabber/FramebufferFrameGrabber.h>

class FramebufferWrapper : public QObject
{
	Q_OBJECT
public:
	FramebufferWrapper(const QString & device, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz);

	const Image<ColorRgb> & getScreenshot();

	///
	/// Starts the timed capturing of screenshots
	///
	void start();

	void stop();

signals:
	void sig_screenshot(const Image<ColorRgb> & screenshot);

private slots:
	///
	/// Performs a single screenshot capture and publishes the capture screenshot on the screenshot signal.
	///
	void capture();

private:
	/// The QT timer to generate capture-publish events
	QTimer _timer;

	/// The grabber for creating screenshots
	FramebufferFrameGrabber   _grabber;

	// image buffers
	Image<ColorRgb>  _screenshot;

};
