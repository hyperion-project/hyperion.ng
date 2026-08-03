#pragma once

#include <clocale>
#include <iostream>

#include <QCoreApplication>
#include <QLocale>
#include <QSharedPointer>
#include <QTimer>

#include <utils/DefaultSignalHandler.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>
#include <utils/MemoryTracker.h>

#include <HyperionConfig.h>

///
/// Generic startup helper for standalone grabber executables.
///
/// Each grabber provides a Traits struct with:
///   - AppType            : QCoreApplication or QGuiApplication
///   - Name               : const char* display name (e.g. "Qt-Grabber")
///   - parseOptions(app)  : returns a grabber-specific options struct
///   - run(app, opts, log, errorManager) : performs the actual work, returns exit code
///   - handleError(log, error)           : called when errorManager emits errorOccurred
///
template <typename Traits>
int runGrabberApp(int argc, char** argv)
{
	setTracingLogPattern();

	QSharedPointer<Logger> log = Logger::getInstance(QStringLiteral(Traits::Name).toUpper());
	Logger::setLogLevel(Logger::LogLevel::Info);

	DefaultSignalHandler::install();
	ErrorManager errorManager;

	typename Traits::AppType app(argc, argv);

	QString const baseName = QCoreApplication::applicationName();
	std::cout << baseName.toStdString() << ":\n"
	          << "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ") - " << BUILD_TIMESTAMP << "\n";

	QObject::connect(&errorManager, &ErrorManager::errorOccurred, [&log](const QString& error) {
		Traits::handleError(log, error);
		QTimer::singleShot(0, []() { QCoreApplication::quit(); });
	});

	// Force locale to have predictable, minimal behavior while still supporting full Unicode.
	setlocale(LC_ALL, "C.UTF-8");
	QLocale::setDefault(QLocale::c());

	auto opts = Traits::parseOptions(app);

	if (opts.debug)
	{
		Logger::setLogLevel(Logger::LogLevel::Debug);
	}

	return Traits::run(app, opts, log, errorManager);
}
