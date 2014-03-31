#include <QMetaType>

#include <grabber/V4L2Wrapper.h>

#include <hyperion/ImageProcessorFactory.h>

V4L2Wrapper::V4L2Wrapper(const std::string &device,
		int input,
		VideoStandard videoStandard,
		PixelFormat pixelFormat,
		int width,
		int height,
		int frameDecimation,
		int pixelDecimation,
		double redSignalThreshold,
		double greenSignalThreshold,
		double blueSignalThreshold,
		Hyperion *hyperion,
		int hyperionPriority) :
	_timeout_ms(1000),
	_priority(hyperionPriority),
	_grabber(device,
			input,
			videoStandard,
			pixelFormat,
			width,
			height,
			frameDecimation,
			pixelDecimation,
			pixelDecimation),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_hyperion(hyperion),
	_ledColors(hyperion->getLedCount(), ColorRgb{0,0,0}),
	_timer()
{
	// set the signal detection threshold of the grabber
	_grabber.setSignalThreshold(
			redSignalThreshold,
			greenSignalThreshold,
			blueSignalThreshold,
			50);

	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");

	// Handle the image in the captured thread using a direct connection
	QObject::connect(
				&_grabber, SIGNAL(newFrame(Image<ColorRgb>)),
				this, SLOT(newFrame(Image<ColorRgb>)),
				Qt::DirectConnection);

	// send color data to Hyperion using a queued connection to handle the data over to the main event loop
	QObject::connect(
				this, SIGNAL(emitColors(int,std::vector<ColorRgb>,int)),
				_hyperion, SLOT(setColors(int,std::vector<ColorRgb>,int)),
				Qt::QueuedConnection);

	// setup the higher prio source checker
	// this will disable the v4l2 grabber when a source with hisher priority is active
	_timer.setInterval(500);
	_timer.setSingleShot(false);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(checkSources()));
	_timer.start();
}

V4L2Wrapper::~V4L2Wrapper()
{
	delete _processor;
}

void V4L2Wrapper::start()
{
	_grabber.start();
}

void V4L2Wrapper::stop()
{
	_grabber.stop();
}

void V4L2Wrapper::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	_grabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void V4L2Wrapper::set3D(VideoMode mode)
{
	_grabber.set3D(mode);
}

void V4L2Wrapper::newFrame(const Image<ColorRgb> &image)
{
	// process the new image
	_processor->process(image, _ledColors);

	// send colors to Hyperion
	emit emitColors(_priority, _ledColors, _timeout_ms);
}

void V4L2Wrapper::checkSources()
{
	QList<int> activePriorities = _hyperion->getActivePriorities();

	for (int x : activePriorities)
	{
		if (x < _priority)
		{
			// found a higher priority source: grabber should be disabled
			_grabber.stop();
			return;
		}
	}

	// no higher priority source was found: grabber should be enabled
	_grabber.start();
}
