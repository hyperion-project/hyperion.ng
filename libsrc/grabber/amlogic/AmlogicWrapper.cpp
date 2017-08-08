// QT includes
#include <QDateTime>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Amlogic grabber includes
#include <grabber/AmlogicWrapper.h>
#include <grabber/AmlogicGrabber.h>


AmlogicWrapper::AmlogicWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("AmLogic", grabWidth, grabHeight, updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new AmlogicGrabber(grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	_ggrabber = _grabber;
}

AmlogicWrapper::~AmlogicWrapper()
{
	delete _grabber;
}

void AmlogicWrapper::action()
{
	unsigned w = _grabber->getImageWidth();
	unsigned h = _grabber->getImageHeight();

	if ( _image.width() != w || _image.height() != h)
	{
		_processor->setSize(w, h);
		_image.resize(w, h);
	}

	// Grab frame into the allocated image
	if (_grabber->grabFrame(_image) >= 0)
	{
		emit emitImage(_priority, _image, _timeout_ms);
		_processor->process(_image, _ledColors);
		setColors(_ledColors, _timeout_ms);
	}
}
