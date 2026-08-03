#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "XcbGrabberOptions.h"

class Logger;
class ErrorManager;

struct XcbGrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "XCB-Grabber";

	static XcbGrabberOptions parseOptions(QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const XcbGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
