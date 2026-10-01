#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "V4L2GrabberOptions.h"

class Logger;
class ErrorManager;

struct V4L2GrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "V42L-Grabber";

	static V4L2GrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const V4L2GrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
