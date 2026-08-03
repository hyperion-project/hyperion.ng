#pragma once

#include <QGuiApplication>
#include <QSharedPointer>

#include "QtGrabberOptions.h"

class Logger;
class ErrorManager;

struct QtGrabberTraits
{
	using AppType = QGuiApplication;

	static constexpr const char* Name = "Qt-Grabber";

	static QtGrabberOptions parseOptions(QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const QtGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
