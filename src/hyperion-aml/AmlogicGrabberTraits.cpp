#include "AmlogicGrabberTraits.h"

#include <QCoreApplication>

#include <grabberapp/GrabberRunner.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "AmlogicGrabberCli.h"
#include "AmlogicWrapper.h"

void AmlogicGrabberTraits::handleError(QSharedPointer<Logger> log, const QString& error)
{
	Error(log, "Error occured: %s", QSTRING_CSTR(error));
}

AmlogicGrabberOptions AmlogicGrabberTraits::parseOptions(const QCoreApplication& app)
{
	return parseAmlogicGrabberOptions(app);
}

int AmlogicGrabberTraits::run(QCoreApplication& /*app*/,
                              const AmlogicGrabberOptions& opts,
                              QSharedPointer<Logger> log,
                              ErrorManager& errorManager)
{
	// Amlogic-Grabber specific: no crop/display support, only fps and size decimation
	AmlogicWrapper grabber(
		opts.fps,
		opts.sizeDecimation);

	return runFlatbufferScreenGrabber(QString::fromUtf8(Name), grabber, opts, log, errorManager);
}
