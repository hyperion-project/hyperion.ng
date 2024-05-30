#include "grabber/dda/DDAWrapper.h"

DDAWrapper::DDAWrapper(int updateRate_Hz, int display, int pixelDecimation, int cropLeft, int cropRight, int cropTop,
                       int cropBottom)
    : GrabberWrapper(GRABBERTYPE, &_grabber, updateRate_Hz), _grabber(display, cropLeft, cropRight, cropTop, cropBottom)

{
	_grabber.setPixelDecimation(pixelDecimation);
}

DDAWrapper::DDAWrapper(const QJsonDocument &grabberConfig)
    : DDAWrapper(GrabberWrapper::DEFAULT_RATE_HZ, 0, GrabberWrapper::DEFAULT_PIXELDECIMATION, 0, 0, 0, 0)
{
	this->handleSettingsUpdate(settings::SYSTEMCAPTURE, grabberConfig);
}

void DDAWrapper::action()
{
	transferFrame(_grabber);
}
