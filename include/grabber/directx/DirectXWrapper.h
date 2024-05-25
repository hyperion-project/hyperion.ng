#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/directx/DirectXGrabber.h>

class DirectXWrapper: public GrabberWrapper
{
public:
	static constexpr const char* GRABBERTYPE = "DirectX";

	///
	/// Constructs the DirectX grabber with a specified grab size and update rate.
	///
	/// @param[in] updateRate_Hz     The image grab rate [Hz]
	/// @param[in] display           Display to be grabbed
	/// @param[in] pixelDecimation   Decimation factor for image [pixels]
	/// @param[in] cropLeft          Remove from left [pixels]
	/// @param[in] cropRight 	     Remove from right [pixels]
	/// @param[in] cropTop           Remove from top [pixels]
	/// @param[in] cropBottom        Remove from bottom [pixels]

	///
	DirectXWrapper( int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
					int display=0,
					int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
					int cropLeft=0, int cropRight=0,
					int cropTop=0, int cropBottom=0
					);

	///
	/// Constructs the QT frame grabber from configuration settings
	///
	DirectXWrapper(const QJsonDocument& grabberConfig = QJsonDocument());

	///
	/// Destructor of this DirectX grabber. Releases any claimed resources.
	///
	virtual ~DirectXWrapper() {};

public slots:
	///
	/// Performs a single frame grab and computes the led-colors
	///
	virtual void action();

private:
	/// The actual grabber
	DirectXGrabber _grabber;
};
