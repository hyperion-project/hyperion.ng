#pragma once

#include <QGuiApplication>
#include <QSharedPointer>

#include "DRMGrabberOptions.h"

class Logger;
class ErrorManager;

struct DRMGrabberTraits
{
	using AppType = QGuiApplication;

	static constexpr const char* Name = "DRM-Grabber";

	static DRMGrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const DRMGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
