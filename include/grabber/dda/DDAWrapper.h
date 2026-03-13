#pragma once

#include <grabber/dda/DDAGrabber.h>
#include <hyperion/GrabberWrapper.h>

class DDAWrapper : public GrabberWrapper
{
public:
	static constexpr const char* GRABBERTYPE = "DDA";

	///
	/// Constructs the DDA frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] display           Display to be grabbed
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	///
	explicit DDAWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
		int display = 0,
		int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
		int cropLeft = 0, int cropRight = 0,
		int cropTop = 0, int cropBottom = 0
	);

	///
	/// Constructs the DDA frame grabber from configuration settings
	///
	explicit DDAWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	///
	/// Starts the grabber, if available
	///
	bool start() override;

	bool open()  override;

public slots:

	///
	/// Performs a single frame grab and computes the LED-colors
	///
	void action();

	void handleSettingsUpdate(settings::type type, const QJsonDocument& grabberConfig) override;

	void handleEvent(Event event) override;

private:
	DDAGrabber _grabber;

	bool _isScreenLocked = false;
};
