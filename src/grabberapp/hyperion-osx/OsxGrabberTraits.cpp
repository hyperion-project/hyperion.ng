#include "OsxGrabberTraits.h"

#include <QCoreApplication>

#include <GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "OsxGrabberCli.h"
#include "OsxWrapper.h"

void OsxGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

OsxGrabberOptions OsxGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseOsxGrabberOptions(app);
}

int OsxGrabberTraits::run(QCoreApplication& /*app*/,
                          const OsxGrabberOptions& opts,
                          QSharedPointer<Logger> log,
                          ErrorManager& errorManager)
{
	// OsX-Grabber specific: the wrapper additionally takes the display index to capture
	OsxWrapper grabber(
		opts.fps,
		opts.display,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
