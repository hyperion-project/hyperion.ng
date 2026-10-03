#include "XcbGrabberTraits.h"

#include <QCoreApplication>

#include <GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "XcbGrabberCli.h"
#include "XcbWrapper.h"

void XcbGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

XcbGrabberOptions XcbGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseXcbGrabberOptions(app);
}

int XcbGrabberTraits::run(QCoreApplication& /*app*/,
                          const XcbGrabberOptions& opts,
                          QSharedPointer<Logger> log,
                          ErrorManager& errorManager)
{
	// Create the XCB grabbing stuff
	XcbWrapper grabber(
		opts.fps,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
