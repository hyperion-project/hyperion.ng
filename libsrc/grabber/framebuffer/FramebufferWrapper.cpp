// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Framebuffer grabber includes
#include <grabber/FramebufferWrapper.h>
#include <grabber/FramebufferFrameGrabber.h>

FramebufferWrapper::FramebufferWrapper(const QString & device, const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, const int priority)
	: GrabberWrapper("FrameBuffer", updateRate_Hz, priority, hyperion::COMP_GRABBER)
	, _grabber(new FramebufferFrameGrabber(device, grabWidth, grabHeight))
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	_ggrabber = _grabber;
	_processor->setSize(grabWidth, grabHeight);
	_image.resize(grabWidth, grabHeight);
}

FramebufferWrapper::~FramebufferWrapper()
{
	delete _grabber;
}

void FramebufferWrapper::action()
{
		unsigned w = _grabber->getImageWidth();
		unsigned h = _grabber->getImageHeight();

		if ( _image.width() != w || _image.height() != h)
		{
			_processor->setSize(w, h);
			_image.resize(w, h);
		}
		
	// Grab frame into the allocated image
	_grabber->grabFrame(_image);

	emit emitImage(_priority, _image, _timeout_ms);
	
	_processor->process(_image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}
