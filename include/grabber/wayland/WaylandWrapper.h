#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/wayland/WaylandGrabber.h>

///
/// The WaylandWrapper uses an instance of the WaylandGrabber to obtain ImageRgb's from the
/// displayed content. This ImageRgb is processed to a ColorRgb for each led and committed to the
/// attached Hyperion.
///
class WaylandWrapper : public GrabberWrapper
{
	Q_OBJECT
public:
	static constexpr const char* GRABBERTYPE = "Wayland";

	explicit WaylandWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
							int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
							int cropLeft = 0, int cropRight = 0,
							int cropTop = 0, int cropBottom = 0);

	///
	/// Constructs the Wayland frame grabber from configuration settings
	///
	explicit WaylandWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	~WaylandWrapper() override;

	bool start() override;
	void stop() override;

public slots:
	void action() override;

private:
	WaylandGrabber _grabber;
};
