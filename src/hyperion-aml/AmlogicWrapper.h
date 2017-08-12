
// QT includes
#include <QTimer>

// Hyperion-Dispmanx includes
#include <grabber/AmlogicGrabber.h>

class AmlogicWrapper : public QObject
{
	Q_OBJECT
public:
	AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz);

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
	AmlogicGrabber   _grabber;

	// image buffers
	Image<ColorRgb>  _screenshot;

};
