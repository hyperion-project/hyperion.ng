#include "DRMGrabberTraits.h"

#include <QCoreApplication>

#include <GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "DRMGrabberCli.h"
#include "DRMWrapper.h"

void DRMGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

DRMGrabberOptions DRMGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseDRMGrabberOptions(app);
}

int DRMGrabberTraits::run(QCoreApplication& /*app*/,
                         const DRMGrabberOptions& opts,
                         QSharedPointer<Logger> log,
                         ErrorManager& errorManager)
{
	// DRM-Grabber specific: the wrapper additionally takes the DRM device index to capture
	DRMWrapper grabber(
		opts.fps,
		opts.deviceIdx,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}