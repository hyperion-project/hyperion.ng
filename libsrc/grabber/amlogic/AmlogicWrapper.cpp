#include <grabber/AmlogicWrapper.h>

AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const unsigned ge2d_mode, const QString device)
	: GrabberWrapper("AmLogic", &_grabber, grabWidth, grabHeight, updateRate_Hz)
	, _grabber(grabWidth, grabHeight, ge2d_mode, device)
{}

void AmlogicWrapper::action()
{
	transferFrame(_grabber);
}
