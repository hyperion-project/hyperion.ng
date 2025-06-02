
#include <cassert>
#include <stdlib.h>
#include <stdio.h>
#include <cstdlib>

#if !defined(__APPLE__) && !defined(_WIN32)
/* prctl is Linux only */
#include <sys/prctl.h>
#endif
// getpid()
#ifdef _WIN32
#include "console.h"
#include <process.h>
#else
#include <unistd.h>
#endif

#include <exception>

#include <QCoreApplication>
#include <QApplication>
#include <QLocale>
#include <QFile>
#include <QString>
#include <QResource>
#include <QDir>
#include <QStringList>
#include <QSystemTrayIcon>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QScopedPointer>

#include "HyperionConfig.h"

#include <utils/Logger.h>
#include <utils/FileUtils.h>
#include <commandline/Parser.h>
#include <commandline/IntOption.h>
#include <utils/DefaultSignalHandler.h>
#include <utils/ErrorManager.h>

#include <db/DBConfigManager.h>
#include <../../include/db/AuthTable.h>

#include "detectProcess.h"

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#endif

#include "hyperiond.h"
#include "systray.h"
#include <events/EventHandler.h>

using namespace commandline;

#define PERM0664 (QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner | QFileDevice::WriteGroup)

QCoreApplication* createApplication(int& argc, char* argv[])
{
	bool isGuiApp = false;
	bool forceNoGui = false;
	// command line
	for (int i = 1; i < argc; ++i)
	{
		if (qstrcmp(argv[i], "--desktop") == 0)
		{
			isGuiApp = true;
		}
		else if (qstrcmp(argv[i], "--service") == 0)
		{
			isGuiApp = false;
			forceNoGui = true;
		}
	}

	// on osx/windows gui always available
#if defined(__APPLE__) || defined(_WIN32)
	isGuiApp = true && !forceNoGui;
#else
	if (!forceNoGui)
	{
		isGuiApp = (getenv("DISPLAY") != NULL && (getenv("XDG_SESSION_TYPE") != NULL || getenv("WAYLAND_DISPLAY") != NULL));
	}
#endif

	if (isGuiApp)
	{
		QApplication* app = new QApplication(argc, argv);
		// add optional library path
		app->addLibraryPath(QApplication::applicationDirPath() + "/../lib");
		app->setApplicationDisplayName("Hyperion");
#ifndef __APPLE__
		app->setWindowIcon(QIcon(":/hyperion-32px.png"));
#endif
		return app;
	}

	QCoreApplication* app = new QCoreApplication(argc, argv);
	app->setApplicationName("Hyperion");
	app->setApplicationVersion(HYPERION_VERSION);
	// add optional library path
	app->addLibraryPath(QApplication::applicationDirPath() + "/../lib");

	return app;
}

int main(int argc, char** argv)
{
	ErrorManager errorManager;
	DefaultSignalHandler::install();

	// initialize main logger and set global log level
	Logger* log = Logger::getInstance("MAIN");
	Logger::setLogLevel(Logger::WARNING);

	// Initialising QCoreApplication
	QScopedPointer<QCoreApplication> app(createApplication(argc, argv));

	// check if we are running already an instance
	// TODO Allow one session per user
	QString processName = QCoreApplication::applicationName();
#ifdef _WIN32
	processName.append(".exe");

	// This prevents INNO Setup from being (un)installed while Hyperion is running!
	CreateMutexA(0, FALSE, "Hyperion");
#endif

	QObject::connect(&errorManager, &ErrorManager::errorOccurred, [&](const QString& error) {
		Error(log, "Error occured: %s", QSTRING_CSTR(error));
		QTimer::singleShot(0, [&app]() { app.get()->quit(); });
		});

	bool isGuiApp = !(qobject_cast<QApplication*>(app.data()) == nullptr) && QSystemTrayIcon::isSystemTrayAvailable();

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	Parser parser("Hyperion Daemon");
	parser.addHelpOption();

	BooleanOption& versionOption = parser.add<BooleanOption>(0x0, "version", "Show version information");
	Option& userDataOption = parser.add<Option>('u', "userdata", "Overwrite user data path, defaults to home directory of current user (%1)", QDir::homePath() + "/.hyperion");
	BooleanOption& resetPassword = parser.add<BooleanOption>(0x0, "resetPassword", "Lost your password? Reset it with this option back to 'hyperion'");
	BooleanOption& readOnlyModeOption = parser.add<BooleanOption>(0x0, "readonlyMode", "Start in read-only mode. No updates will be written to the database");
	BooleanOption& deleteDB = parser.add<BooleanOption>(0x0, "deleteDatabase", "Start all over? This Option will delete the database");
	Option& importConfig = parser.add<Option>(0x0, "importConfig", "Replace the current configuration database by a new configuration");
	Option& exportConfigPath = parser.add<Option>(0x0, "exportConfig", "Export the current configuration database, defaults to home directory of current user (%1)", QDir::homePath() + "/.hyperion//archive");
	BooleanOption& silentOption = parser.add<BooleanOption>('s', "silent", "Do not print any outputs");
	BooleanOption& verboseOption = parser.add<BooleanOption>('v', "verbose", "Increase verbosity");
	BooleanOption& debugOption = parser.add<BooleanOption>('d', "debug", "Show debug messages");
#ifdef WIN32
	BooleanOption& consoleOption = parser.add<BooleanOption>('c', "console", "Open a console window to view log output");
#endif
	parser.add<BooleanOption>(0x0, "desktop", "Show systray on desktop");
	parser.add<BooleanOption>(0x0, "service", "Force hyperion to start as console service");
#if defined(ENABLE_EFFECTENGINE)
	Option& exportEfxOption = parser.add<Option>(0x0, "export-effects", "Export effects to given path");
#endif

	/* Internal options, invisible to help */
	BooleanOption& waitOption = parser.addHidden<BooleanOption>(0x0, "wait-hyperion", "Do not exit if other Hyperion instances are running, wait them to finish");

	parser.process(*qApp);

#ifdef WIN32
	bool isShowConsole = parser.isSet(consoleOption);
	openConsole(isShowConsole);
#endif

	if (parser.isSet(versionOption))
	{
		std::cout
			<< "Hyperion Ambilight Deamon" << "\n"
			<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << "\n"
			<< "\tBuild Time: " << __DATE__ << " " << __TIME__ << "\n";

		return EXIT_SUCCESS;
	}

	if (!parser.isSet(waitOption))
	{
		if (getProcessIdsByProcessName(processName).size() > 1)
		{
			Error(log, "The Hyperion Daemon is already running, abort start");

			// use the first non-localhost IPv4 address
			QList<QHostAddress> const allNetworkAddresses{ QNetworkInterface::allAddresses() };
			auto it = std::find_if(allNetworkAddresses.begin(), allNetworkAddresses.end(),
				[](const QHostAddress& address)
				{
					return !address.isLoopback() && (address.protocol() == QAbstractSocket::IPv4Protocol);
				});

			if (it != allNetworkAddresses.end())
			{
				std::cout << "Access the Hyperion User-Interface for configuration and control via:" << "\n";
				std::cout << "http://" << it->toString().toStdString() << ":8090" << "\n";

				QHostInfo const hostInfo = QHostInfo::fromName(it->toString());
				if (hostInfo.error() == QHostInfo::NoError)
				{
					QString const hostname = hostInfo.hostName();
					std::cout << "http://" << hostname.toStdString() << ":8090" << "\n";
				}
			}
			return EXIT_SUCCESS;
		}
	}
	else
	{
		while (getProcessIdsByProcessName(processName).size() > 1)
		{
			QThread::msleep(100);
		}
	}

	int logLevelCheck = 0;
	if (parser.isSet(silentOption))
	{
		Logger::setLogLevel(Logger::OFF);
		logLevelCheck++;
	}

	if (parser.isSet(verboseOption))
	{
		Logger::setLogLevel(Logger::INFO);
		logLevelCheck++;
	}

	if (parser.isSet(debugOption))
	{
		Logger::setLogLevel(Logger::DEBUG);
		logLevelCheck++;
	}

	Info(log, "Hyperion %s, %s, built: %s:%s", HYPERION_VERSION, HYPERION_BUILD_ID, __DATE__, __TIME__);
	Debug(log, "QtVersion [%s]", QT_VERSION_STR);

	if (logLevelCheck > 1)
	{
		emit errorManager.errorOccurred("Options --silent --verbose --debug cannot be used all together.");
		return EXIT_FAILURE;
	}

#if defined(ENABLE_EFFECTENGINE)
	if (parser.isSet(exportEfxOption))
	{
		Q_INIT_RESOURCE(EffectEngine);
		QDir const sourceDir(":/effects/");
		QDir const destinationDir(exportEfxOption.value(parser));

		// Create destination if it does not exist
		if (!destinationDir.exists())
		{
			std::cout << "Creating target directory: " << destinationDir.absolutePath().toStdString() << '\n';
			if (!QDir().mkpath(destinationDir.absolutePath()))
			{
				emit errorManager.errorOccurred(QString("Failed to create directory: '%1'.").arg(destinationDir.absolutePath()));
				return EXIT_FAILURE;
			}
		}

		if (sourceDir.exists())
		{
			std::cout << "Extract to folder: " << destinationDir.absolutePath().toStdString() << '\n';
			const QStringList filenames = sourceDir.entryList(QStringList() << "*", QDir::Files, QDir::Name | QDir::IgnoreCase);
			for (const QString& filename : filenames)
			{
				QString const sourceFilePath = sourceDir.absoluteFilePath(filename);
				QString const destinationFilePath = destinationDir.absoluteFilePath(filename);

				if (QFile::exists(destinationFilePath))
				{
					QFile::remove(destinationFilePath);
				}

				if (Logger::getLogLevel() == Logger::DEBUG)
				{
					std::cout << "Copy \"" << sourceFilePath.toStdString() << "\" -> \"" << destinationFilePath.toStdString() << "\"" << '\n';
				}

				std::cout << "Extract: " << filename.toStdString() << " ... ";
				if (QFile::copy(sourceFilePath, destinationFilePath))
				{
					QFile::setPermissions(destinationFilePath, PERM0664);
					std::cout << "OK" << '\n';
				}
				else
				{
					std::cerr << "Error copying [" << sourceFilePath.toStdString() << " -> [" << destinationFilePath.toStdString() << "]" << '\n';

					emit errorManager.errorOccurred("Failed to copy effect(s) to target directory.");
					return EXIT_FAILURE;
				}
			}
			return EXIT_SUCCESS;
		}

		emit errorManager.errorOccurred(QString("Can not export to %1.").arg(exportEfxOption.getCString(parser)));
		return EXIT_FAILURE;
	}
#endif

	bool readonlyMode = false;
	QString const userDataPath(userDataOption.value(parser));
	QDir const userDataDirectory(userDataPath);


	if (parser.isSet(readOnlyModeOption))
	{
		readonlyMode = true;
		Debug(log, "Force readonlyMode");
	}

	DBManager::initializeDatabase(userDataDirectory, readonlyMode);

	Info(log, "Hyperion configuration and user data location: '%s'", QSTRING_CSTR(userDataDirectory.absolutePath()));

	QFileInfo const dbFile(DBManager::getFileInfo());

	DBConfigManager configManager;
	if (dbFile.exists())
	{
		if (!dbFile.isReadable())
		{
			emit errorManager.errorOccurred(QString("Configuration database '%1' is not readable. Please setup permissions correctly.").arg(dbFile.absoluteFilePath()));
			return EXIT_FAILURE;
		}

		if (!dbFile.isWritable())
		{
			readonlyMode = true;
		}

		if (parser.isSet(exportConfigPath))
		{
			QString path = exportConfigPath.value(parser);
			if (path.isEmpty())
			{
				path = userDataDirectory.absolutePath().append("/archive");
			}

			if (!configManager.exportJson(path))
			{
				emit errorManager.errorOccurred("Configuration export failed.");
				return EXIT_FAILURE;
			}

			return EXIT_SUCCESS;
		}
	}
	else
	{
		if (parser.isSet(exportConfigPath))
		{
			emit errorManager.errorOccurred(QString("The configuration cannot be exported. The database file '%1' does not exist.").arg(dbFile.absoluteFilePath()));
			return EXIT_FAILURE;
		}

		if (!userDataDirectory.mkpath(dbFile.absolutePath()))
		{
			if (!userDataDirectory.isReadable() || !dbFile.isWritable())
			{
				emit errorManager.errorOccurred(QString("The user data path '%1' cannot be created or nor is readable/writable. Please setup permissions correctly.").arg(userDataDirectory.absolutePath()));
				return EXIT_FAILURE;
			}
		}
	}

	// reset Password without spawning daemon
	if (parser.isSet(resetPassword))
	{
		if (readonlyMode)
		{
			emit errorManager.errorOccurred(QString("Password reset is not possible. Hyperion's database '%1' is not writable.").arg(dbFile.absoluteFilePath()));
			return EXIT_FAILURE;
		}

		QScopedPointer<AuthTable> const table(new AuthTable());
		if (!table->resetHyperionUser())
		{
			emit errorManager.errorOccurred("Failed to reset password.");
			return EXIT_FAILURE;
		}

		Info(log, "Password reset successful.");
		return EXIT_SUCCESS;
	}

	// delete database before start
	if (parser.isSet(deleteDB))
	{
		if (readonlyMode)
		{
			emit errorManager.errorOccurred(QString("Deleting the configuration database failed. Hyperion's database '%1' is not writable.").arg(dbFile.absoluteFilePath()));
			return EXIT_FAILURE;
		}

		if (QFile::exists(dbFile.absoluteFilePath()))
		{
			if (!QFile::remove(dbFile.absoluteFilePath()))
			{
				emit errorManager.errorOccurred("Failed to delete Database.");
				return EXIT_FAILURE;
			}

			Info(log, "Configuration database deleted successfully.");
		}
		else
		{
			Warning(log, "Configuration database '%s' does not exist.", QSTRING_CSTR(dbFile.absoluteFilePath()));
		}
	}

	QString const configFile(importConfig.value(parser));
	if (!configFile.isEmpty())
	{
		if (readonlyMode)
		{
			emit errorManager.errorOccurred(QString("Configuration import failed. Hyperion's database '%1' is not writable.").arg(dbFile.absoluteFilePath()));

			return EXIT_FAILURE;
		}

		if (!configManager.importJson(configFile).first)
		{
			emit errorManager.errorOccurred("Configuration import failed.");
			return EXIT_FAILURE;
		}
	}

	if (!configManager.addMissingDefaults().first)
	{
		emit errorManager.errorOccurred("Updating configuration database with missing defaults failed.");
		return EXIT_FAILURE;
	}

	if (!configManager.migrateConfiguration().first)
	{
		emit errorManager.errorOccurred("Migrating the configuration database failed.");
		return EXIT_FAILURE;
	}

	if (!configManager.validateConfiguration().first)
	{
		if (!configManager.updateConfiguration().first)
		{
			emit errorManager.errorOccurred("Invalid configuration database. Correcting the configuration database failed.");
			return EXIT_FAILURE;
		}
	}

	if (!configFile.isEmpty())
	{
		Info(log, "Configuration imported sucessfully. You can start Hyperion now.");
		return EXIT_SUCCESS;
	}

	Info(log, "Starting Hyperion in %sGUI mode, DB is %s", isGuiApp ? "" : "non-", readonlyMode ? "read-only" : "read/write");

	if (readonlyMode)
	{
		Warning(log, "The database file '%s' is set not writable. Hyperion starts in read-only mode. Configuration updates will not be persisted.", QSTRING_CSTR(dbFile.absoluteFilePath()));
	}

	QScopedPointer<HyperionDaemon> hyperiond;
	try
	{
		hyperiond.reset(new HyperionDaemon(userDataDirectory.absolutePath(), qApp, bool(logLevelCheck)));
	}
	catch (std::exception& e)
	{
		Error(log, "Hyperion Daemon aborted: %s", e.what());
	}

	int exitCode{ EXIT_FAILURE };
	// run the application
	if (isGuiApp)
	{
		Info(log, "Start Systray menu");
		QApplication::setQuitOnLastWindowClosed(false);
		SysTray tray(hyperiond.get());
		tray.hide();
		exitCode = (qobject_cast<QApplication*>(app.data()))->exec();
	}
	else
	{
		exitCode = app->exec();
	}

	Info(log, "Application ended with code %d", exitCode);

#ifdef _WIN32
	if (parser.isSet(consoleOption))
	{
		system("pause");
	}
#endif

	Logger::deleteInstance();

	return exitCode;
}
