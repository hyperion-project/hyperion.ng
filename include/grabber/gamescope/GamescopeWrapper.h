#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/gamescope/GamescopeGrabber.h>

///
/// The GamescopeWrapper uses an instance of the GamescopeGrabber to obtain frames from
/// gamescope's composited output. This Image is processed to a ColorRgb for each led
/// and committed to the attached Hyperion.
///
class GamescopeWrapper : public GrabberWrapper
{
	Q_OBJECT
public:
	static constexpr const char* GRABBERTYPE = "Gamescope";

	explicit GamescopeWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
		int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
		int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0);

	explicit GamescopeWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

public slots:
	void action() override;

private:
	GamescopeGrabber _grabber;
};
