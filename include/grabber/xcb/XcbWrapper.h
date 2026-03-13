#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/xcb/XcbGrabber.h>

// some include of xorg defines "None" this is also used by QT and has to be undefined to avoid collisions
#ifdef None
	#undef None
#endif

class XcbWrapper: public GrabberWrapper
{
public:

	static constexpr const char* GRABBERTYPE = "XCB";

	///
	/// Constructs the XCB frame grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]
	///
	explicit XcbWrapper(	int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
				int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
				int cropLeft=0, int cropRight=0,
				int cropTop=0, int cropBottom=0
				);

	///
	/// Constructs the XCB frame grabber from configuration settings
	///
	explicit XcbWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	~XcbWrapper() override;

	///
	/// Starts the grabber, if available
	///
	bool start() override;

	///
	/// Starts the grabber which produces led values with the specified update rate
	///
	bool open() override;

public slots:
	void action() override;

private:
	XcbGrabber _grabber;

	bool _init;
};
