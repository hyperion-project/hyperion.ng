
// QT includes
#include <QThread>

// Hyperion-Dispmanx includes
#include <grabber/AmlogicGrabber.h>

class AmlogicWrapper : public QObject
{
	Q_OBJECT
public:
	AmlogicWrapper(unsigned grabWidth, unsigned grabHeight);

	const Image<ColorRgb> & getScreenshot();

	///
	/// Starts the threaded capturing of screenshots
	///
	void start();

	void stop();

signals:
	void sig_screenshot(const Image<ColorRgb> & screenshot);

private slots:
	///
	/// Performs screenshot captures and publishes the capture screenshot on the screenshot signal.
	///
	void capture();

private:
	/// The QT thread to generate capture-publish events
	QThread _thread;

	/// The grabber for creating screenshots
	AmlogicGrabber   _grabber;

	// image buffers
	Image<ColorRgb>  _screenshot;
};
