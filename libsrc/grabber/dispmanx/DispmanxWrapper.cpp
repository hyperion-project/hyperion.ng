#include <grabber/DispmanxWrapper.h>


DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("Dispmanx", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _image_rgba(grabWidth, grabHeight)
	, _grabber(grabWidth, grabHeight)
{
	_ggrabber = &_grabber;
}

DispmanxWrapper::~DispmanxWrapper()
{
}

void DispmanxWrapper::action()
{
	_grabber.grabFrame(_image_rgba);
	_image_rgba.toRgb(_image);
	setImage();
}
