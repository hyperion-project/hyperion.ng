#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "DispmanxGrabberOptions.h"

class Logger;
class ErrorManager;

struct DispmanxGrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "DispmanX";

	static DispmanxGrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const DispmanxGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
