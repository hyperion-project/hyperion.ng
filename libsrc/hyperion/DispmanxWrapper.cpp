// QT includes
#include <QDebug>
#include <QDateTime>

// Hyperion includes
#include <hyperion/DispmanxWrapper.h>
#include <hyperion/ImageProcessorFactory.h>


// Local-Hyperion includes
#include "DispmanxFrameGrabber.h"

DispmanxWrapper::DispmanxWrapper() :
	_timer(),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_frameGrabber(new DispmanxFrameGrabber(64, 64))
{
	_timer.setInterval(100);
	_timer.setSingleShot(false);

	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(action()));
}

DispmanxWrapper::~DispmanxWrapper()
{
	delete _frameGrabber;
	delete _processor;
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
