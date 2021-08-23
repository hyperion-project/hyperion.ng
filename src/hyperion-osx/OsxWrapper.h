
// QT includes
#include <QTimer>

#include <grabber/OsxFrameGrabber.h>
#include <hyperion/GrabberWrapper.h>

class OsxWrapper : public QObject
{
	Q_OBJECT
public:

	OsxWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
				int display=kCGDirectMainDisplay,
				int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
				int cropLeft=0, int cropRight=0,
				int cropTop=0, int cropBottom=0
				);

	const Image<ColorRgb> & getScreenshot();

	///
	/// Starts the timed capturing of screenshots
	///
	void start();

	void stop();

	bool displayInit();

signals:
	void sig_screenshot(const Image<ColorRgb> & screenshot);

public slots:
	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(VideoMode videoMode);

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
