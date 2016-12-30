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
		const int priority)
	: GrabberWrapper("V4L2:"+device, priority, hyperion::COMP_V4L)
	, _timeout_ms(1000)
	, _grabber(device,
			input,
			videoStandard,
			pixelFormat,
			width,
			height,
			frameDecimation,
			pixelDecimation,
			pixelDecimation)
	, _ledColors(Hyperion::getInstance()->getLedCount(), ColorRgb{0,0,0})
{
	// set the signal detection threshold of the grabber
	_grabber.setSignalThreshold( redSignalThreshold, greenSignalThreshold, blueSignalThreshold, 50);

	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");

	// Handle the image in the captured thread using a direct connection
	QObject::connect(&_grabber, SIGNAL(newFrame(Image<ColorRgb>)), this, SLOT(newFrame(Image<ColorRgb>)), Qt::DirectConnection);

	QObject::connect(&_grabber, SIGNAL(readError(const char*)), this, SLOT(readError(const char*)), Qt::DirectConnection);

	// send color data to Hyperion using a queued connection to handle the data over to the main event loop
// 	QObject::connect(
// 				this, SIGNAL(emitColors(int,std::vector<ColorRgb>,int)),
// 				_hyperion, SLOT(setColors(int,std::vector<ColorRgb>,int)),
// 				Qt::QueuedConnection);

	
	// setup the higher prio source checker
	// this will disable the v4l2 grabber when a source with higher priority is active
	_timer.setInterval(500);
}

V4L2Wrapper::~V4L2Wrapper()
{
}

bool V4L2Wrapper::start()
{
	return ( _grabber.start() && GrabberWrapper::start());
}

void V4L2Wrapper::stop()
{
	_grabber.stop();
	GrabberWrapper::stop();
}

void V4L2Wrapper::setCropping(int cropLeft, int cropRight, int cropTop, int cropBottom)
{
	_grabber.setCropping(cropLeft, cropRight, cropTop, cropBottom);
}

void V4L2Wrapper::setSignalDetectionOffset(double verticalMin, double horizontalMin, double verticalMax, double horizontalMax)
{
	_grabber.setSignalDetectionOffset(verticalMin, horizontalMin, verticalMax, horizontalMax);
}


void V4L2Wrapper::set3D(VideoMode mode)
{
	_grabber.set3D(mode);
}

void V4L2Wrapper::newFrame(const Image<ColorRgb> &image)
{
	emit emitImage(_priority, image, _timeout_ms);

	// process the new image
	_processor->process(image, _ledColors);
	setColors(_ledColors, _timeout_ms);
}

void V4L2Wrapper::readError(const char* err)
{
	Error(_log, "stop grabber, because reading device failed. (%s)", err);
	stop();
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

void V4L2Wrapper::action()
{
	checkSources();
}
