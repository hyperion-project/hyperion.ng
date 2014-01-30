
// QT includes
#include <QTimer>

// Hyperion-X11 includes
#include "X11Grabber.h"

class X11Wrapper : public QObject
{
	Q_OBJECT
public:
	X11Wrapper(const unsigned cropHorizontal, const unsigned cropVertical, const unsigned pixelDecimation);

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
	/// Performs a single screenshot capture and publishes the capture screenshot on the screenshot
	/// signal.
	///
	void capture();

private:
	/// The QT timer to generate capture-publish events
	QTimer _timer;

	/// The grabber for creating screenshots
	X11Grabber _grabber;
};
