// QT includes
#include <QDebug>
#include <QDateTime>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageProcessorFactory.h>
#include <hyperion/ImageProcessor.h>

// Local-dispmanx includes
#include <dispmanx-grabber/DispmanxWrapper.h>
#include "DispmanxFrameGrabber.h"


DispmanxWrapper::DispmanxWrapper(const unsigned grabWidth, const unsigned grabHeight, const unsigned updateRate_Hz, Hyperion * hyperion) :
	_updateInterval_ms(1000/updateRate_Hz),
	_timeout_ms(2 * _updateInterval_ms),
	_priority(128),
	_timer(),
	_image(grabWidth, grabHeight),
	_frameGrabber(new DispmanxFrameGrabber(grabWidth, grabHeight)),
	_processor(ImageProcessorFactory::getInstance().newImageProcessor()),
	_ledColors(hyperion->getLedCount(), RgbColor::BLACK),
	_hyperion(hyperion)
{
	// Configure the timer to generate events every n milliseconds
	_timer.setInterval(_updateInterval_ms);
	_timer.setSingleShot(false);

	_processor->setSize(grabWidth, grabHeight);

	// Connect the QTimer to this
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(action()));
}

DispmanxWrapper::~DispmanxWrapper()
{
	// Cleanup used resources (ImageProcessor and FrameGrabber)
	delete _processor;
	delete _frameGrabber;
}

void DispmanxWrapper::start()
{
	// Start the timer with the pre configured interval
	_timer.start();
}

void DispmanxWrapper::action()
{
	// Grab frame into the allocated image
	_frameGrabber->grabFrame(_image);

	_processor->process(_image, _ledColors);

	_hyperion->setColors(_priority, _ledColors, _timeout_ms);
}
void DispmanxWrapper::stop()
{
	// Stop the timer, effectivly stopping the process
	_timer.stop();
}
