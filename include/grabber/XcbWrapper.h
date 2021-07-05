#pragma once

#include <hyperion/GrabberWrapper.h>
#include <grabber/XcbGrabber.h>

// some include of xorg defines "None" this is also used by QT and has to be undefined to avoid collisions
#ifdef None
	#undef None
#endif

class XcbWrapper: public GrabberWrapper
{
public:
	XcbWrapper(	int updateRate_Hz=GrabberWrapper::DEFAULT_RATE_HZ,
				int pixelDecimation=GrabberWrapper::DEFAULT_PIXELDECIMATION,
				int cropLeft=0, int cropRight=0,
				int cropTop=0, int cropBottom=0
				);

	~XcbWrapper() override;

public slots:
	void action() override;

private:
	XcbGrabber _grabber;

	bool _init;
};
