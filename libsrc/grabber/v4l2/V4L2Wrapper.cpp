#include <QMetaType>

#include <grabber/V4L2Wrapper.h>

// qt
#include <QTimer>

V4L2Wrapper::V4L2Wrapper(const QString &device,
		VideoStandard videoStandard,
		PixelFormat pixelFormat,
		int pixelDecimation )
	: GrabberWrapper("V4L2:"+device, &_grabber, 0, 0, 10)
	, _grabber(device,
			videoStandard,
			pixelFormat,
			pixelDecimation)
{
	_ggrabber = &_grabber;

	// register the image type
	qRegisterMetaType<Image<ColorRgb>>("Image<ColorRgb>");

	// Handle the image in the captured thread using a direct connection
	connect(&_grabber, SIGNAL(newFrame(Image<ColorRgb>)), this, SLOT(newFrame(Image<ColorRgb>)), Qt::DirectConnection);
	connect(&_grabber, SIGNAL(readError(const char*)), this, SLOT(readError(const char*)), Qt::DirectConnection);
	
	connect(this, &V4L2Wrapper::componentStateChanged, _ggrabber, &Grabber::componentStateChanged);
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

void V4L2Wrapper::setSignalThreshold(double redSignalThreshold, double greenSignalThreshold, double blueSignalThreshold)
{
	_grabber.setSignalThreshold( redSignalThreshold, greenSignalThreshold, blueSignalThreshold, 50);
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
	emit systemImage(_grabberName, image);
}

void V4L2Wrapper::readError(const char* err)
{
	Error(_log, "stop grabber, because reading device failed. (%s)", err);
	stop();
}

void V4L2Wrapper::action()
{
	// dummy as v4l get notifications from stream
}

void V4L2Wrapper::setSignalDetectionEnable(bool enable)
{
	_grabber.setSignalDetectionEnable(enable);
}

bool V4L2Wrapper::getSignalDetectionEnable()
{
	return _grabber.getSignalDetectionEnabled();
}

void V4L2Wrapper::setDeviceVideoStandard(QString device, VideoStandard videoStandard)
{
	_grabber.setDeviceVideoStandard(device, videoStandard);
}
