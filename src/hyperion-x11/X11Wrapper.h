
// QT includes
#include <QTimer>

// Hyperion-X11 includes
#include <grabber/X11Grabber.h>

//Utils includes
#include <utils/GrabbingMode.h>
#include <utils/VideoMode.h>

class X11Wrapper : public QObject
{
	Q_OBJECT
public:
	X11Wrapper(int grabInterval, bool useXGetImage, int cropLeft, int cropRight, int cropTop, int cropBottom, int horizontalPixelDecimation, int verticalPixelDecimation);

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
	/// Set the grabbing mode
	/// @param[in] mode The new grabbing mode
	///
	void setGrabbingMode(const GrabbingMode mode);

	///
	/// Set the video mode (2D/3D)
	/// @param[in] mode The new video mode
	///
	void setVideoMode(const VideoMode videoMode);

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
	
	Image<ColorRgb>  _screenshot;
};
