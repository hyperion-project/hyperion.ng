#include <QMetaType>

#include <grabber/V4L2Wrapper.h>

#include <hyperion/ImageProcessorFactory.h>

V4L2Wrapper::V4L2Wrapper(const std::string &device,
		int input,
		VideoStandard videoStandard,
		int width,
		int height,
		int frameDecimation,
		int pixelDecimation,
		Hyperion *hyperion,
		int hyperionPriority) :
	_timeout_ms(1000),
	_priority(hyperionPriority),
	_grabber(device,
			 input,
			 videoStandard,
			 width,
			 height,
			 frameDecimation,
			 pixelDecimation,
			 pixelDecimation),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_hyperion(hyperion),
	_ledColors(hyperion->getLedCount(), ColorRgb{0,0,0})
{
	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	// connect the new frame signal using a queued connection, because it will be called from a different thread
	QObject::connect(&_grabber, SIGNAL(newFrame(Image<ColorRgb>)), this, SLOT(newFrame(Image<ColorRgb>)), Qt::QueuedConnection);
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
	// TODO: add a signal detector

	// process the new image
	_processor->process(image, _ledColors);

	// send colors to Hyperion
	_hyperion->setColors(_priority, _ledColors, _timeout_ms);
}

