#include "QtGrabberTraits.h"

#include <QCoreApplication>

#include <grabberapp/GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "QtGrabberCli.h"
#include "QtWrapper.h"

void QtGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

QtGrabberOptions QtGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseQtGrabberOptions(app);
}

int QtGrabberTraits::run(QCoreApplication& /*app*/,
                         const QtGrabberOptions& opts,
                         QSharedPointer<Logger> log,
                         ErrorManager& errorManager)
{
	// Qt-Grabber specific: the wrapper additionally takes the display index to capture
	QtWrapper grabber(
		opts.fps,
		opts.display,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
