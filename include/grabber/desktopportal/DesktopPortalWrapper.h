#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/desktopportal/DesktopPortalGrabber.h>

///
/// The DesktopPortalWrapper uses an instance of the DesktopPortalGrabber to obtain frames from a
/// normal (non-gamescope) Wayland desktop session via xdg-desktop-portal. This Image is
/// processed to a ColorRgb for each led and committed to the attached Hyperion.
///
class DesktopPortalWrapper : public GrabberWrapper
{
	Q_OBJECT
public:
	static constexpr const char* GRABBERTYPE = "DesktopPortal";

	explicit DesktopPortalWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
		int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
		int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0);

	explicit DesktopPortalWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

public slots:
	void action() override;

private:
	DesktopPortalGrabber _grabber;
};
