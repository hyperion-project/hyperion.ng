#include "DispmanxGrabberTraits.h"

#include <QCoreApplication>

#include <GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "DispmanxGrabberCli.h"
#include "DispmanxWrapper.h"

void DispmanxGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

DispmanxGrabberOptions DispmanxGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseDispmanxGrabberOptions(app);
}

int DispmanxGrabberTraits::run(QCoreApplication& /*app*/,
                               const DispmanxGrabberOptions& opts,
                               QSharedPointer<Logger> log,
                               ErrorManager& errorManager)
{
	// Create the dispmanx grabbing stuff
	DispmanxWrapper grabber(
		opts.fps,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
