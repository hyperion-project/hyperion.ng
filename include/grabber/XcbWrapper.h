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
	XcbWrapper(int cropLeft, int cropRight, int cropTop, int cropBottom, int pixelDecimation, const unsigned updateRate_Hz);
	~XcbWrapper() override;

public slots:
	virtual void action();

private:
	XcbGrabber _grabber;

	bool _init;
};
