#include "V4L2GrabberTraits.h"

#include <QCoreApplication>

#include <GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "V4L2GrabberCli.h"
#include "V4L2Wrapper.h"

void V4L2GrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

V4L2GrabberOptions V4L2GrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseV4L2GrabberOptions(app);
}

int V4L2GrabberTraits::run(QCoreApplication& /*app*/,
                           const V4L2GrabberOptions& opts,
                           QSharedPointer<Logger> log,
                           ErrorManager& errorManager)
{
	if (!opts.valid)
	{
		emit errorManager.errorOccurred(opts.error);
		return 1;
	}

	// V4L2-Grabber specific: all device/encoding/signal-detection settings are applied
	// once up-front in the wrapper's constructor.
	V4L2Wrapper grabber(opts);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
