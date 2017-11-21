// Hyperion includes
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>
#include <hyperion/GrabberWrapper.h>
#include <hyperion/Grabber.h>
#include <HyperionConfig.h>

GrabberWrapper::GrabberWrapper(QString grabberName, Grabber * ggrabber, unsigned width, unsigned height, const unsigned updateRate_Hz, const int priority, hyperion::Components grabberComponentId)
	: _grabberName(grabberName)
	, _hyperion(Hyperion::getInstance())
	, _priority(priority)
	, _timer()
	, _updateInterval_ms(1000/updateRate_Hz)
	, _timeout_ms(2 * _updateInterval_ms)
	, _log(Logger::getInstance(grabberName))
	, _forward(true)
	, _processor(ImageProcessorFactory::getInstance().newImageProcessor())
	, _grabberComponentId(grabberComponentId)
	, _ggrabber(ggrabber)
	, _image(0,0)
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
	, _imageProcessorEnabled(true)
{
	_timer.setSingleShot(false);
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);

	_image.resize(width, height);
	_processor->setSize(width, height);

	_forward = _hyperion->getForwarder()->protoForwardingEnabled();
	_hyperion->getComponentRegister().componentStateChanged(hyperion::COMP_BLACKBORDER, _processor->blackBorderDetectorEnabled());
	qRegisterMetaType<hyperion::Components>("hyperion::Components");

	connect(_hyperion, SIGNAL(imageToLedsMappingChanged(int)), _processor, SLOT(setLedMappingType(int)));
	connect(_hyperion, SIGNAL(componentStateChanged(hyperion::Components,bool)), this, SLOT(componentStateChanged(hyperion::Components,bool)));
	connect(_hyperion, SIGNAL(videoMode(VideoMode)), this, SLOT(setVideoMode(VideoMode)));
	connect(this, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), _hyperion, SLOT(setImage(int, const Image<ColorRgb>&, const int)) );
	connect(&_timer, SIGNAL(timeout()), this, SLOT(actionWrapper()));

}

GrabberWrapper::~GrabberWrapper()
{
	stop();
	Debug(_log,"Close grabber: %s", QSTRING_CSTR(_grabberName));
	delete _processor;
}

bool GrabberWrapper::start()
{
	// Start the timer with the pre configured interval
	_timer.start();
	_hyperion->registerPriority(_grabberName, _priority);
	return _timer.isActive();

}

void GrabberWrapper::stop()
{
	// Stop the timer, effectivly stopping the process
	_timer.stop();
	_hyperion->unRegisterPriority(_grabberName);
}

void GrabberWrapper::actionWrapper()
{
	_ggrabber->setEnabled(_hyperion->isCurrentPriority(_priority));
	action();
}

void GrabberWrapper::componentStateChanged(const hyperion::Components component, bool enable)
{
	if (component == _grabberComponentId)
	{
		if (_timer.isActive() != enable)
		{
			if (enable) start();
			else        stop();

			_forward = _hyperion->getForwarder()->protoForwardingEnabled();

			if ( enable == _timer.isActive() )
			{
				Info(_log, "grabber change state to %s", (_timer.isActive() ? "enabled" : "disabled") );
			}
			else
			{
				WarningIf( enable, _log, "enable grabber failed");
			}
		}
		_hyperion->getComponentRegister().componentStateChanged(component, _timer.isActive());
	}

	if (component == hyperion::COMP_BLACKBORDER)
	{
		if (_processor->blackBorderDetectorEnabled() != enable)
		{
			_processor->enableBlackBorderDetector(enable);
			Info(_log, "bb detector change state to %s", (_processor->blackBorderDetectorEnabled() ? "enabled" : "disabled") );
		}
		_hyperion->getComponentRegister().componentStateChanged(component, _processor->blackBorderDetectorEnabled());
	}
}

void GrabberWrapper::setColors(const std::vector<ColorRgb> &ledColors, const int timeout_ms)
{
	_hyperion->setColors(_priority, ledColors, timeout_ms, true, _grabberComponentId);
}

QStringList GrabberWrapper::availableGrabbers()
{
	QStringList grabbers;

	#ifdef ENABLE_DISPMANX
	grabbers << "dispmanx";
	#endif

	#ifdef ENABLE_V4L2
	grabbers << "v4l2";
	#endif

	#ifdef ENABLE_FB
	grabbers << "framebuffer";
	#endif

	#ifdef ENABLE_AMLOGIC
	grabbers << "amlogic";
	#endif

	#ifdef ENABLE_OSX
	grabbers << "osx";
	#endif

	#ifdef ENABLE_X11
	grabbers << "x11";
	#endif

	return grabbers;
}


void GrabberWrapper::setVideoMode(const VideoMode mode)
{
	if (_ggrabber != nullptr)
	{
		Info(_log,"setvideomode");
		_ggrabber->setVideoMode(mode);
	}
}

void GrabberWrapper::setCropping(unsigned cropLeft, unsigned cropRight, unsigned cropTop, unsigned cropBottom)
{
	_ggrabber->setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void GrabberWrapper::setImageProcessorEnabled(bool enable)
{
	_imageProcessorEnabled = enable;
}
