#pragma once

#include <QCoreApplication>
#include <QSharedPointer>

#include "FramebufferGrabberOptions.h"

class Logger;
class ErrorManager;

struct FramebufferGrabberTraits
{
	using AppType = QCoreApplication;

	static constexpr const char* Name = "Framebuffer";

	static FramebufferGrabberOptions parseOptions(const QCoreApplication& app);

	static int run(QCoreApplication& app,
	               const FramebufferGrabberOptions& opts,
	               QSharedPointer<Logger> log,
	               ErrorManager& errorManager);

	static void handleError(QSharedPointer<Logger> log, const QString& error);
};
