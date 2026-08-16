#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "X11GrabberOptions.h"

class Logger;
class ErrorManager;

struct X11GrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "X11-Grabber";

	static X11GrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const X11GrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
