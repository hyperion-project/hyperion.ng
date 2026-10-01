#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "AmlogicGrabberOptions.h"

class Logger;
class ErrorManager;

struct AmlogicGrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "Amlogic";

	static AmlogicGrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const AmlogicGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
