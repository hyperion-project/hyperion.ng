
// QT includes
#include <QTimer>

#include <grabber/AmlogicGrabber.h>
#include <hyperion/GrabberWrapper.h>

class AmlogicWrapper : public QObject
{
	Q_OBJECT
public:
	AmlogicWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
					int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
					int cropLeft=0, int cropRight=0,
					int cropTop=0, int cropBottom=0
					);

	const Image<ColorRgb> & getScreenshot();

	///
	/// Starts the threaded capturing of screenshots
	///
	void start();

	void stop();

	bool screenInit();

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
	/// Performs screenshot captures and publishes the capture screenshot on the screenshot signal.
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
