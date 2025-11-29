#pragma once

#include <QJsonObject>
#include <QStringLiteral>

#include <hyperion/GrabberWrapper.h>
#include <grabber/qt/QtGrabber.h>

///
/// The QtWrapper uses QtFramework API's to get a picture from system
///
class QtWrapper: public GrabberWrapper
{

public:
	static constexpr const char* GRABBERTYPE = "Qt";

	///
	/// Constructs the QT frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] display           Display to be grabbed
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	///
	explicit QtWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
			   int display=0,
			   int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
			   int cropLeft=0, int cropRight=0,
			   int cropTop=0, int cropBottom=0
			   );

	///
	/// Constructs the QT frame grabber from configuration settings
	///
	explicit QtWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	///
	/// Starts the grabber, if available
	///
	bool start() override;

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	void action() override;

private:
	/// The actual grabber
	QtGrabber _grabber;
};
