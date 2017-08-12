
// QT includes
#include <QTimer>

// Hyperion-Dispmanx includes
#include <grabber/DispmanxFrameGrabber.h>

class DispmanxWrapper : public QObject
{
	Q_OBJECT
public:
	DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight,
		const VideoMode& videoMode,
		const unsigned cropLeft, const unsigned cropRight,
		const unsigned cropTop, const unsigned cropBottom,
		const unsigned updateRate_Hz);

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
	DispmanxFrameGrabber _grabber;
	Image<ColorRgb>   _screenshot;
};
