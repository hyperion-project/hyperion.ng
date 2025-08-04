#pragma once

#include <grabber/dda/DDAGrabber.h>
#include <hyperion/GrabberWrapper.h>

class DDAWrapper : public GrabberWrapper
{
public:
	static constexpr const char* GRABBERTYPE = "DDA";

	DDAWrapper(int updateRate_Hz = GrabberWrapper::DEFAULT_RATE_HZ,
		int display = 0,
		int pixelDecimation = GrabberWrapper::DEFAULT_PIXELDECIMATION,
		int cropLeft = 0, int cropRight = 0, int cropTop = 0, int cropBottom = 0
	);

	DDAWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	virtual ~DDAWrapper() {};

	///
	/// Starts the grabber, if available
	///
	bool start() override;

	///
	/// Starts the grabber which produces LED values with the specified update rate
	///
	bool open() override;

public slots:
	virtual void action();

private:
	DDAGrabber _grabber;
};
