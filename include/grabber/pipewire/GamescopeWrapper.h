#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/pipewire/GamescopeGrabber.h>

///
/// @brief GrabberWrapper adapter for GamescopeGrabber.
///
/// The wrapper drives the capture timer loop and forwards frames to Hyperion
/// via GrabberWrapper::transferFrame(), exactly as every other wrapper does.
///
class GamescopeWrapper : public GrabberWrapper
{
	Q_OBJECT
public:
	static constexpr const char* GRABBERTYPE = "Gamescope";

	explicit GamescopeWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
	                          int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
	                          int cropLeft = 0, int cropRight = 0,
	                          int cropTop  = 0, int cropBottom = 0);

	///
	/// @brief Constructs the Gamescope grabber from configuration settings.
	///
	explicit GamescopeWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	~GamescopeWrapper() override;

	bool start()  override;
	void stop()   override;

public slots:
	void action() override;

private:
	GamescopeGrabber _grabber;
};

