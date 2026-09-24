#include "FramebufferGrabberTraits.h"

#include <QCoreApplication>

#include <grabberapp/GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "FramebufferGrabberCli.h"
#include "FramebufferWrapper.h"

void FramebufferGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

FramebufferGrabberOptions FramebufferGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseFramebufferGrabberOptions(app);
}

int FramebufferGrabberTraits::run(QCoreApplication& /*app*/,
                                  const FramebufferGrabberOptions& opts,
                                  QSharedPointer<Logger> log,
                                  ErrorManager& errorManager)
{
	// Framebuffer-Grabber specific: the wrapper additionally takes the device index to capture
	FramebufferWrapper grabber(
		opts.fps,
		opts.deviceIdx,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
