#pragma once

#include <QTimer>

#include <grabber/pipewire/WaylandGrabber.h>
#include <hyperion/GrabberWrapper.h>

class WLWrapper : public QObject
{
	Q_OBJECT
public:
	explicit WLWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
					   int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
					   int cropLeft = 0, int cropRight = 0,
					   int cropTop = 0, int cropBottom = 0);

	~WLWrapper() override;

	///
	/// Captures a single frame. The portal session is asynchronous (and may ask the user to
	/// select a screen), so this waits until the first frame arrives or a timeout expires.
	///
	const Image<ColorRgb>& getScreenshot();

	///
	/// Starts the portal session and the timed capturing of screenshots
	///
	void start();

	void stop();

	bool screenInit();

signals:
	void sig_screenshot(const Image<ColorRgb>& screenshot);

public slots:
	void setVideoMode(VideoMode videoMode);

private slots:
	void capture();

private:
	QTimer _timer;
	WaylandGrabber _grabber;
	Image<ColorRgb> _screenshot;
};
