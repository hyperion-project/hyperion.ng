
// QT includes
#include <QTimer>

// Hyperion-Dispmanx includes
#include <grabber/OsxFrameGrabber.h>

class OsxWrapper : public QObject
{
	Q_OBJECT
public:
	OsxWrapper(unsigned display, unsigned grabWidth, unsigned grabHeight, unsigned updateRate_Hz);

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
	OsxFrameGrabber   _grabber;

	// image buffers
	Image<ColorRgb>  _screenshot;

};
