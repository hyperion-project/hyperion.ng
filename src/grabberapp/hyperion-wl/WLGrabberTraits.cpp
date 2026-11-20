#include "WLGrabberTraits.h"

#include <QCoreApplication>

#include <GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "WLGrabberCli.h"
#include "WLWrapper.h"

void WLGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

WLGrabberOptions WLGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseWLGrabberOptions(app);
}

int WLGrabberTraits::run(QCoreApplication& /*app*/,
                         const WLGrabberOptions& opts,
                         QSharedPointer<Logger> log,
                         ErrorManager& errorManager)
{
	WLWrapper grabber(
		opts.fps,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}