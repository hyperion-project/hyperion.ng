#include <QMetaType>

#include <grabber/V4L2Wrapper.h>

#include <hyperion/ImageProcessorFactory.h>

V4L2Wrapper::V4L2Wrapper(const QString &device,
		int input,
		VideoStandard videoStandard,
		PixelFormat pixelFormat,
		unsigned width,
		unsigned height,
		int frameDecimation,
		int pixelDecimation,
		double redSignalThreshold,
		double greenSignalThreshold,
		double blueSignalThreshold,
		const int priority)
	: GrabberWrapper("V4L2:"+device, &_grabber, width, height, 8, priority, hyperion::COMP_V4L)
	, _grabber(device,
			input,
			videoStandard,
			pixelFormat,
			width,
			height,
			frameDecimation,
			pixelDecimation,
			pixelDecimation)
{
	// set the signal detection threshold of the grabber
	_grabber.setSignalThreshold( redSignalThreshold, greenSignalThreshold, blueSignalThreshold, 50);
	_ggrabber = &_grabber;

	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");
	qRegisterMetaType<std::vector<ColorRgb>>("std::vector<ColorRgb>");
	qRegisterMetaType<hyperion::Components>("hyperion::Components");

	// Handle the image in the captured thread using a direct connection
	QObject::connect(&_grabber, SIGNAL(newFrame(Image<ColorRgb>)), this, SLOT(newFrame(Image<ColorRgb>)), Qt::DirectConnection);
	QObject::connect(&_grabber, SIGNAL(readError(const char*)), this, SLOT(readError(const char*)), Qt::DirectConnection);

	_timer.setInterval(500);
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
	if ( _hyperion->isCurrentPriority(_priority))
	{
		_grabber.start();
	}
	else
	{
		_grabber.stop();
	}
}

void V4L2Wrapper::action()
{
	checkSources();
}

void V4L2Wrapper::setSignalDetectionEnable(bool enable)
{
	_grabber.setSignalDetectionEnable(enable);
}

bool V4L2Wrapper::getSignalDetectionEnable()
{
	return _grabber.getSignalDetectionEnabled();
}
