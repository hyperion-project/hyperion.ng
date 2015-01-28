#pragma once

// QT includes
#include <QObject>
#include <QTimer>

// Utils includes
#include <utils/Image.h>
#include <utils/ColorRgb.h>
#include <utils/ColorRgba.h>
#include <utils/GrabbingMode.h>
#include <utils/VideoMode.h>

// Forward class declaration
class OsxFrameGrabber;
class Hyperion;
class ImageProcessor;

///
/// The OsxWrapper uses an instance of the OsxFrameGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and commmited to the
/// attached Hyperion.
///
class OsxWrapper: public QObject
{
	Q_OBJECT
public:
	///
	/// Constructs the osx frame grabber with a specified grab size and update rate.
	///
	/// @param[in] display Index of the display to grab
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	/// @param[in] hyperion  The instance of Hyperion used to write the led values
	///
	OsxWrapper(const unsigned display, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, Hyperion * hyperion);

	///
	/// Destructor of this osx frame grabber. Releases any claimed resources.
	///
	virtual ~OsxWrapper();

public slots:
	///
	/// Starts the grabber wich produces led values with the specified update rate
	///
	void start();

	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action();

	///
	/// Stops the grabber
	///
	void stop();

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

private:
	/// The update rate [Hz]
	const int _updateInterval_ms;
	/// The timeout of the led colors [ms]
	const int _timeout_ms;
	/// The priority of the led colors
	const int _priority;

	/// The timer for generating events with the specified update rate
	QTimer _timer;

	/// The image used for grabbing frames
	Image<ColorRgb> _image;
	/// The actual grabber
	OsxFrameGrabber * _frameGrabber;
	/// The processor for transforming images to led colors
	ImageProcessor * _processor;

	/// The list with computed led colors
	std::vector<ColorRgb> _ledColors;

	/// Pointer to Hyperion for writing led values
	Hyperion * _hyperion;
};
