#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "OsxGrabberOptions.h"

class Logger;
class ErrorManager;

struct OsxGrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "OsX-Grabber";

	static OsxGrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const OsxGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
