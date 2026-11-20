#pragma once

#include <QGuiApplication>
#include <QSharedPointer>

#include "WLGrabberOptions.h"

class Logger;
class ErrorManager;

struct WLGrabberTraits
{
	using AppType = QGuiApplication;

	static constexpr const char* Name = "Wayland-Grabber";

	static WLGrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const WLGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
