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

#define PERM0664 QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner | QFileDevice::WriteGroup

#ifndef _WIN32
void signal_handler(int signum)
{
	// Hyperion Managment instance
	HyperionIManager *_hyperion = HyperionIManager::getInstance();

	if (signum == SIGCHLD)
	{
		// only quit when a registered child process is gone
		// currently this feature is not active ...
		return;
	}
	else if (signum == SIGUSR1)
	{
		if (_hyperion != nullptr)
		{
			_hyperion->toggleStateAllInstances(false);
		}
		return;
	}
	else if (signum == SIGUSR2)
	{
		if (_hyperion != nullptr)
		{
			_hyperion->toggleStateAllInstances(true);
		}
		return;
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
		// if x11, then test if xserver is available
		#if defined(ENABLE_X11)
		Display* dpy = XOpenDisplay(NULL);
		if (dpy != NULL)
		{
			XCloseDisplay(dpy);
			isGuiApp = true;
		}
		#elif defined(ENABLE_XCB)
			int screen_num;
			xcb_connection_t * connection = xcb_connect(nullptr, &screen_num);
			if (!xcb_connection_has_error(connection))
			{
				isGuiApp = true;
			}
			xcb_disconnect(connection);
		#endif
	}
#endif

	if (isGuiApp)
	{
		QApplication* app = new QApplication(argc, argv);
		// add optional library path
		app->addLibraryPath(QApplication::applicationDirPath() + "/../lib");
		app->setApplicationDisplayName("Hyperion");
		app->setWindowIcon(QIcon(":/hyperion-icon-32px.png"));
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
#ifndef _WIN32
	setenv("AVAHI_COMPAT_NOWARN", "1", 1);
#endif
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

	bool isGuiApp = (qobject_cast<QApplication *>(app.data()) != 0 && QSystemTrayIcon::isSystemTrayAvailable());

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
	Option        & exportEfxOption     = parser.add<Option>        (0x0, "export-effects", "Export effects to given path");

	/* Internal options, invisible to help */
	BooleanOption & waitOption          = parser.addHidden<BooleanOption> (0x0, "wait-hyperion", "Do not exit if other Hyperion instances are running, wait them to finish");

	parser.process(*qApp);

	if (!parser.isSet(waitOption))
	{
		if (getProcessIdsByProcessName(processName).size() > 1)
		{
			Error(log, "The Hyperion Daemon is already running, abort start");
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

#ifdef WIN32
	if (parser.isSet(consoleOption))
	{
		CreateConsole();
	}
#endif

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

	if (parser.isSet(versionOption))
	{
		std::cout
			<< "Hyperion Ambilight Deamon" << std::endl
			<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
			<< "\tBuild Time: " << __DATE__ << " " << __TIME__ << std::endl;

		return 0;
	}

	if (parser.isSet(exportEfxOption))
	{
		Q_INIT_RESOURCE(EffectEngine);
		QDir directory(":/effects/");
		QDir destDir(exportEfxOption.value(parser));
		if (directory.exists() && destDir.exists())
		{
			std::cout << "extract to folder: " << std::endl;
			QStringList filenames = directory.entryList(QStringList() << "*", QDir::Files, QDir::Name | QDir::IgnoreCase);
			QString destFileName;
			for (const QString & filename : filenames)
			{
				destFileName = destDir.dirName()+"/"+filename;
				if (QFile::exists(destFileName))
					QFile::remove(destFileName);

				std::cout << "Extract: " << filename.toStdString() << " ... ";
				if (QFile::copy(QString(":/effects/")+filename, destFileName))
				{
					QFile::setPermissions(destFileName, PERM0664 );
					std::cout << "ok" << std::endl;
				}
				else
				{
					 std::cout << "error, aborting" << std::endl;
					 return 1;
				}
			}
			return 0;
		}

		Error(log, "can not export to %s",exportEfxOption.getCString(parser));
		return 1;
	}

	int rc = 1;

	try
	{
		// handle and create userDataPath for user data, default path is home directory + /.hyperion
		// NOTE: No further checks inside Hyperion. FileUtils::writeFile() will resolve permission errors and others that occur during runtime
		QString userDataPath(userDataOption.value(parser));
		QDir mDir(userDataPath);
		QFileInfo mFi(userDataPath);
		if(!mDir.mkpath(userDataPath) || !mFi.isWritable() || !mDir.isReadable())
			throw std::runtime_error("The user data path '"+mDir.absolutePath().toStdString()+"' can't be created or isn't read/writeable. Please setup permissions correctly!");

		Info(log, "Set user data path to '%s'", QSTRING_CSTR(mDir.absolutePath()));

		// reset Password without spawning daemon
		if(parser.isSet(resetPassword))
		{
			AuthTable* table = new AuthTable(userDataPath);
			if(table->resetHyperionUser()){
				Info(log,"Password reset successfull");
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
			const QString dbFile = mDir.absolutePath() + "/db/hyperion.db";
			if (QFile::exists(dbFile))
			{
				if (!QFile::remove(dbFile))
				{
					Info(log,"Failed to delete Database!");
					exit(1);
				}
			}
		}

		HyperionDaemon* hyperiond = nullptr;
		try
		{
			hyperiond = new HyperionDaemon(userDataPath, qApp, bool(logLevelCheck));
		}
		catch (std::exception& e)
		{
			Error(log, "Hyperion Daemon aborted: %s", e.what());
			throw;
		}

		// run the application
		if (isGuiApp)
		{
			Info(log, "start systray");
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
	return rc;
}
