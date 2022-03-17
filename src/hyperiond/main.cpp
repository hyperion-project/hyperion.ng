#include <cassert>
#include <csignal>
#include <stdlib.h>
#include <stdio.h>

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

#include "HyperionConfig.h"

#include <utils/Logger.h>
#include <utils/FileUtils.h>
#include <commandline/Parser.h>
#include <commandline/IntOption.h>
#include <utils/DefaultSignalHandler.h>
#include <../../include/db/AuthTable.h>

#include "detectProcess.h"

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#endif

#include "hyperiond.h"
#include "systray.h"

using namespace commandline;

#define PERM0664 (QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner | QFileDevice::WriteGroup)

#ifndef _WIN32
void signal_handler(int signum)
{
	// Hyperion Managment instance
	HyperionIManager *_hyperion = HyperionIManager::getInstance();

	if (signum == SIGCHLD)
	{
		// only quit when a registered child process is gone
		// currently this feature is not active ...
	}
	else if (signum == SIGUSR1)
	{
		if (_hyperion != nullptr)
		{
			_hyperion->toggleStateAllInstances(false);
		}
	}
	else if (signum == SIGUSR2)
	{
		if (_hyperion != nullptr)
		{
			_hyperion->toggleStateAllInstances(true);
		}
	}
}
#endif

QCoreApplication* createApplication(int &argc, char *argv[])
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
	isGuiApp = true && ! forceNoGui;
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
		app->setWindowIcon(QIcon(":/hyperion-icon-32px.png"));
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
	// initialize main logger and set global log level
	Logger *log = Logger::getInstance("MAIN");
	Logger::setLogLevel(Logger::WARNING);

	// check if we are running already an instance
	// TODO Allow one session per user
	#ifdef _WIN32
		const char* processName = "hyperiond.exe";
	#else
		const char* processName = "hyperiond";
	#endif

	// Initialising QCoreApplication
	QScopedPointer<QCoreApplication> app(createApplication(argc, argv));

	bool isGuiApp = !(qobject_cast<QApplication *>(app.data()) == nullptr) && QSystemTrayIcon::isSystemTrayAvailable();

	DefaultSignalHandler::install();

#ifndef _WIN32
	signal(SIGCHLD, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);
#endif
	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	Parser parser("Hyperion Daemon");
	parser.addHelpOption();

	BooleanOption & versionOption       = parser.add<BooleanOption> (0x0, "version", "Show version information");
	Option        & userDataOption      = parser.add<Option>        ('u', "userdata", "Overwrite user data path, defaults to home directory of current user (%1)", QDir::homePath() + "/.hyperion");
	BooleanOption & resetPassword       = parser.add<BooleanOption> (0x0, "resetPassword", "Lost your password? Reset it with this option back to 'hyperion'");
	BooleanOption & deleteDB            = parser.add<BooleanOption> (0x0, "deleteDatabase", "Start all over? This Option will delete the database");
	BooleanOption & silentOption        = parser.add<BooleanOption> ('s', "silent", "Do not print any outputs");
	BooleanOption & verboseOption       = parser.add<BooleanOption> ('v', "verbose", "Increase verbosity");
	BooleanOption & debugOption         = parser.add<BooleanOption> ('d', "debug", "Show debug messages");
#ifdef WIN32
	BooleanOption & consoleOption       = parser.add<BooleanOption> ('c', "console", "Open a console window to view log output");
#endif
	                                      parser.add<BooleanOption> (0x0, "desktop", "Show systray on desktop");
	                                      parser.add<BooleanOption> (0x0, "service", "Force hyperion to start as console service");
#if defined(ENABLE_EFFECTENGINE)
	Option        & exportEfxOption     = parser.add<Option>        (0x0, "export-effects", "Export effects to given path");
#endif

	/* Internal options, invisible to help */
	BooleanOption & waitOption          = parser.addHidden<BooleanOption> (0x0, "wait-hyperion", "Do not exit if other Hyperion instances are running, wait them to finish");

	parser.process(*qApp);

#ifdef WIN32
	if (parser.isSet(consoleOption))
	{
		CreateConsole();
	}
#endif

	if (parser.isSet(versionOption))
	{
		std::cout
			<< "Hyperion Ambilight Deamon" << std::endl
			<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
			<< "\tBuild Time: " << __DATE__ << " " << __TIME__ << std::endl;

		return 0;
	}

	if (!parser.isSet(waitOption))
	{
		if (getProcessIdsByProcessName(processName).size() > 1)
		{
			Error(log, "The Hyperion Daemon is already running, abort start");

			// use the first non-localhost IPv4 address, IPv6 are not supported by Yeelight currently
			for (const auto& address : QNetworkInterface::allAddresses())
			{
				if (!address.isLoopback() && (address.protocol() == QAbstractSocket::IPv4Protocol))
				{
					std::cout << "Access the Hyperion User-Interface for configuration and control via:" << std::endl;
					std::cout << "http://" << address.toString().toStdString() << ":8090" << std::endl;

					QHostInfo hostInfo = QHostInfo::fromName(address.toString());
					if (hostInfo.error() == QHostInfo::NoError)
					{
						QString hostname = hostInfo.hostName();
						std::cout << "http://" << hostname.toStdString() << ":8090" << std::endl;
					}
					break;
				}
			}
			return 0;
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

	if (logLevelCheck > 1)
	{
		Error(log, "aborting, because options --silent --verbose --debug can't be used together");
		return 0;
	}

#if defined(ENABLE_EFFECTENGINE)
	if (parser.isSet(exportEfxOption))
	{
		Q_INIT_RESOURCE(EffectEngine);
		QDir directory(":/effects/");
		QDir destDir(exportEfxOption.value(parser));
		if (directory.exists() && destDir.exists())
		{
			std::cout << "Extract to folder: " << destDir.absolutePath().toStdString() << std::endl;
			QStringList filenames = directory.entryList(QStringList() << "*", QDir::Files, QDir::Name | QDir::IgnoreCase);
			QString destFileName;
			for (const QString & filename : qAsConst(filenames))
			{
				destFileName = destDir.dirName()+"/"+filename;
				if (QFile::exists(destFileName))
				{
					QFile::remove(destFileName);
				}

				std::cout << "Extract: " << filename.toStdString() << " ... ";
				if (QFile::copy(QString(":/effects/")+filename, destFileName))
				{
					QFile::setPermissions(destFileName, PERM0664 );
					std::cout << "OK" << std::endl;
				}
				else
				{
					std::cout << "Error, aborting" << std::endl;
					return 1;
				}
			}
			return 0;
		}

		Error(log, "Can not export to %s",exportEfxOption.getCString(parser));
		return 1;
	}
#endif

	int rc = 1;
	bool readonlyMode = false;

	QString userDataPath(userDataOption.value(parser));

	QDir userDataDirectory(userDataPath);
	QFileInfo dbFile(userDataDirectory.absolutePath() +"/db/hyperion.db");

	try
	{
		if (dbFile.exists())
		{
			if (!dbFile.isReadable())
			{
				throw std::runtime_error("Configuration database '" + dbFile.absoluteFilePath().toStdString() + "' is not readable. Please setup permissions correctly!");
			}

			if (!dbFile.isWritable())
			{
				readonlyMode = true;
			}
		}
		else
		{
			if (!userDataDirectory.mkpath(dbFile.absolutePath()))
			{
				if (!userDataDirectory.isReadable() || !dbFile.isWritable())
				{
					throw std::runtime_error("The user data path '" + userDataDirectory.absolutePath().toStdString() + "' can't be created or isn't read/writeable. Please setup permissions correctly!");
				}
			}
		}

		// reset Password without spawning daemon
		if(parser.isSet(resetPassword))
		{
			if ( readonlyMode )
			{
				Error(log,"Password reset is not possible. The user data path '%s' is not writeable.", QSTRING_CSTR(userDataDirectory.absolutePath()));
				throw std::runtime_error("Password reset failed");
			}

			AuthTable* table = new AuthTable(userDataDirectory.absolutePath());
			if(table->resetHyperionUser()){
				Info(log,"Password reset successful");
				delete table;
				exit(0);
			} else {
				Error(log,"Failed to reset password!");
				delete table;
				exit(1);
			}
		}

		// delete database before start
		if(parser.isSet(deleteDB))
		{
			if ( readonlyMode )
			{
				Error(log,"Deleting the configuration database is not possible. The user data path '%s' is not writeable.", QSTRING_CSTR(dbFile.absolutePath()));
				throw std::runtime_error("Deleting the configuration database failed");
			}

			if (QFile::exists(dbFile.absoluteFilePath()))
			{
				if (!QFile::remove(dbFile.absoluteFilePath()))
				{
					Info(log,"Failed to delete Database!");
					exit(1);
				}
				else
				{
					Info(log,"Configuration database deleted successfully.");
				}
			}
			else
			{
				Warning(log,"Configuration database [%s] does not exist!", QSTRING_CSTR(dbFile.absoluteFilePath()));
			}
		}

		Info(log,"Starting Hyperion [%sGUI mode] - %s, %s, built: %s:%s", isGuiApp ? "": "non-", HYPERION_VERSION, HYPERION_BUILD_ID, __DATE__, __TIME__);
		Debug(log,"QtVersion [%s]", QT_VERSION_STR);

		if ( !readonlyMode )
		{
			Info(log, "Set user data path to '%s'", QSTRING_CSTR(userDataDirectory.absolutePath()));
		}
		else
		{
			Warning(log,"The user data path '%s' is not writeable. Hyperion starts in read-only mode. Configuration updates will not be persisted!", QSTRING_CSTR(userDataDirectory.absolutePath()));
		}

		HyperionDaemon* hyperiond = nullptr;
		try
		{
			hyperiond = new HyperionDaemon(userDataDirectory.absolutePath(), qApp, bool(logLevelCheck), readonlyMode);
		}
		catch (std::exception& e)
		{
			Error(log, "Hyperion Daemon aborted: %s", e.what());
			throw;
		}

		// run the application
		if (isGuiApp)
		{
			Info(log, "Start Systray menu");
			QApplication::setQuitOnLastWindowClosed(false);
			SysTray tray(hyperiond);
			tray.hide();
			rc = (qobject_cast<QApplication *>(app.data()))->exec();
		}
		else
		{
			rc = app->exec();
		}
		Info(log, "Application closed with code %d", rc);
		delete hyperiond;
	}
	catch (std::exception& e)
	{
		Error(log, "Hyperion aborted: %s", e.what());
	}

	// delete components
	Logger::deleteInstance();

#ifdef _WIN32
	if (parser.isSet(consoleOption))
	{
		system("pause");
	}
#endif

	return rc;
}
