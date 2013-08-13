// QT includes
#include <QDebug>
#include <QDateTime>

// Hyperion includes
#include <hyperion/DispmanxWrapper.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>


// Local-Hyperion includes
#include "DispmanxFrameGrabber.h"

DispmanxWrapper::DispmanxWrapper() :
	_timer(),
	_frameGrabber(new DispmanxFrameGrabber(64, 64)),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor())
{
	_timer.setInterval(100);
	_timer.setSingleShot(false);

	_processor->setSize(64, 64);

	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(action()));
}

DispmanxWrapper::~DispmanxWrapper()
{
	delete _processor;
	delete _frameGrabber;
}

void DispmanxWrapper::start()
{
	_timer.start();
}

void DispmanxWrapper::action()
{
	qDebug() << "[" << QDateTime::currentDateTimeUtc() << "] Grabbing frame";
	RgbImage image(64, 64);
	_frameGrabber->grabFrame(image);

	//_processor->
}
void DispmanxWrapper::stop()
{
	_timer.stop();
}
