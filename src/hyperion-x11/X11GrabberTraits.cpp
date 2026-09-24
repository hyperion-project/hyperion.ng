#include "X11GrabberTraits.h"

#include <QCoreApplication>

#include <grabberapp/GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "X11GrabberCli.h"
#include "X11Wrapper.h"

void X11GrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

X11GrabberOptions X11GrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseX11GrabberOptions(app);
}

int X11GrabberTraits::run(QCoreApplication& /*app*/,
                          const X11GrabberOptions& opts,
                          QSharedPointer<Logger> log,
                          ErrorManager& errorManager)
{
	// Create the X11 grabbing stuff
	X11Wrapper grabber(
		opts.fps,
		opts.sizeDecimation,
		opts.cropLeft,
		opts.cropRight,
		opts.cropTop,
		opts.cropBottom);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
