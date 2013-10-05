#pragma once

// QT includes
#include <QObject>
#include <QTimer>

// Utils includes
#include <utils/RgbColor.h>
#include <utils/RgbImage.h>
#include <utils/GrabbingMode.h>

// Forward class declaration
class DispmanxFrameGrabber;
class Hyperion;
class ImageProcessor;

///
/// The DispmanxWrapper uses an instance of the DispmanxFrameGrabber to obtain RgbImage's from the
/// displayed content. This RgbImage is processed to a RgbColor for each led and commmited to the
/// attached Hyperion.
///
class DispmanxWrapper: public QObject
{
	Q_OBJECT
public:
	///
	/// Constructs the dispmanx frame grabber with a specified grab size and update rate.
	///
	/// @param[in] grabWidth  The width of the grabbed image [pixels]
	/// @param[in] grabHeight  The height of the grabbed images [pixels]
	/// @param[in] updateRate_Hz  The image grab rate [Hz]
	/// @param[in] hyperion  The instance of Hyperion used to write the led values
	///
	DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, Hyperion * hyperion);

	///
	/// Destructor of this dispmanx frame grabber. Releases any claimed resources.
	///
	virtual ~DispmanxWrapper();

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

private:
	/// The update rate [Hz]
	const int _updateInterval_ms;
	/// The timeout of the led colors [ms]
	const int _timeout_ms;
	/// The priority of the led colors [ms]
	const int _priority;

	/// The timer for generating events with the specified update rate
	QTimer _timer;

	/// The image used for grabbing frames
	RgbImage _image;
	/// The actual grabber
	DispmanxFrameGrabber * _frameGrabber;
	/// The processor for transforming images to led colors
	ImageProcessor * _processor;

	/// The list with computed led colors
	std::vector<RgbColor> _ledColors;

	/// Pointer to Hyperion for writing led values
	Hyperion * _hyperion;
};
