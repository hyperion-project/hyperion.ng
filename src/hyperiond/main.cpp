#include <cassert>
#include <csignal>
#include <unistd.h>
#include <stdlib.h>

#ifndef __APPLE__
/* prctl is Linux only */
#include <sys/prctl.h>
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

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#endif

#include "hyperiond.h"
#include "systray.h"

using namespace commandline;

#define PERM0664 QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner | QFileDevice::WriteGroup

void signal_handler(const int signum)
{
	if(signum == SIGCHLD)
	{
		// only quit when a registered child process is gone
		// currently this feature is not active ...
		return;
	}
	QCoreApplication::quit();

	// reset signal handler to default (in case this handler is not capable of stopping)
	signal(signum, SIG_DFL);
}


void startNewHyperion(int parentPid, std::string hyperionFile, std::string configFile)
{
	pid_t childPid = fork(); // child pid should store elsewhere for later use
	if ( childPid == 0 )
	{
		sleep(3);
		execl(hyperionFile.c_str(), hyperionFile.c_str(), "--parent", QString::number(parentPid).toStdString().c_str(), configFile.c_str(), NULL);
		exit(0);
	}
}

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
#if defined(__APPLE__) || defined(__WIN32__)
	isGuiApp = true && ! forceNoGui;
#else
	if (!forceNoGui)
	{
		// if x11, then test if xserver is available
		#ifdef ENABLE_X11
		Display* dpy = XOpenDisplay(NULL);
		if (dpy != NULL)
		{
			XCloseDisplay(dpy);
			isGuiApp = true;
		}
		#endif
	}
#endif

	if (isGuiApp)
	{
		QApplication* app = new QApplication(argc, argv);
		app->setApplicationDisplayName("Hyperion");
		return  app;
	}

	QCoreApplication* app = new QCoreApplication(argc, argv);
	app->setApplicationName("Hyperion");
	app->setApplicationVersion(HYPERION_VERSION);
	return app;
}

int main(int argc, char** argv)
{
	setenv("AVAHI_COMPAT_NOWARN", "1", 1);

	// initialize main logger and set global log level
	Logger* log = Logger::getInstance("MAIN");
	Logger::setLogLevel(Logger::WARNING);

	// Initialising QCoreApplication
    QScopedPointer<QCoreApplication> app(createApplication(argc, argv));
	bool isGuiApp = (qobject_cast<QApplication *>(app.data()) != 0 && QSystemTrayIcon::isSystemTrayAvailable());

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGPIPE, signal_handler);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	Parser parser("Hyperion Daemon");
	parser.addHelpOption();

	BooleanOption & versionOption       = parser.add<BooleanOption>(0x0, "version", "Show version information");
	Option        & rootPathOption      = parser.add<Option>       (0x0, "rootPath", "Overwrite root path for all hyperion user files, defaults to home directory of current user");
	IntOption     &  parentOption       = parser.add<IntOption>    ('p', "parent", "pid of parent hyperiond"); // 2^22 is the max for Linux
	BooleanOption &  silentOption       = parser.add<BooleanOption>('s', "silent", "do not print any outputs");
	BooleanOption & verboseOption       = parser.add<BooleanOption>('v', "verbose", "Increase verbosity");
	BooleanOption &   debugOption       = parser.add<BooleanOption>('d', "debug", "Show debug messages");
	parser.add<BooleanOption>(0x0, "desktop", "show systray on desktop");
	parser.add<BooleanOption>(0x0, "service", "force hyperion to start as console service");
	Option        & exportConfigOption  = parser.add<Option>       (0x0, "export-config", "export default config to file");
	Option        & exportEfxOption     = parser.add<Option>       (0x0, "export-effects", "export effects to given path");

	parser.addPositionalArgument("config-files", QCoreApplication::translate("main", "Configuration file"), "config.file");

    parser.process(*qApp);

	QStringList configFiles = parser.positionalArguments();

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
		Error(log, "aborting, because options --silent --verbose --debug can't used together");
		return 0;
	}

	if (parser.isSet(versionOption))
	{
	std::cout
		<< "Hyperion Ambilight Deamon (" << getpid() << ")" << std::endl
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
			foreach (const QString & filename, filenames)
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

	// handle rootPath for user data, default path is home directory + /.hyperion
	QString rootPath = QDir::homePath() + "/.hyperion";
	if (parser.isSet(rootPathOption))
	{
		QDir rDir(rootPathOption.value(parser));
		if(!rDir.mkpath(rootPathOption.value(parser)))
		{
			Error(log, "Can't create root path '%s', falling back to home directory", QSTRING_CSTR(rDir.absolutePath()));
		}
		else
		{
			rootPath = rDir.absolutePath();
		}
	}
	// create /.hyperion folder for default path, check if the directory is read/writeable
	// Note: No further checks inside Hyperion. FileUtils::writeFile() will resolve permission errors and others that occur during runtime
	QDir mDir(rootPath);
	QFileInfo mFi(rootPath);
	if(!mDir.mkpath(rootPath) || !mFi.isWritable() || !mDir.isReadable())
	{
		throw std::runtime_error("The specified root path can't be created or isn't read/writeable. Please setup the permissions correctly!");
	}

	// determine name of config file, defaults to hyperion_main.json
	// create config folder
	QString cPath(rootPath+"/config");
	QDir().mkpath(rootPath+"/config");
	if (configFiles.size() > 0)
	{
		// use argument config file
		// check if file has a path and ends with .json
		if(configFiles[0].contains("/"))
			throw std::runtime_error("Don't provide a path to config file, just a config name is allowed!");
		if(!configFiles[0].endsWith(".json"))
			configFiles[0].append(".json");

		configFiles.prepend(cPath+"/"+configFiles[0]);
	}
	else
	{
		// use default config file
		configFiles.append(cPath+"/hyperion_main.json");
	}

	bool exportDefaultConfig = false;
	bool exitAfterExportDefaultConfig = false;
	QString exportConfigFileTarget;
	if (parser.isSet(exportConfigOption))
	{
		exportDefaultConfig = true;
		exitAfterExportDefaultConfig = true;
		exportConfigFileTarget = exportConfigOption.value(parser);
	}
	else if ( ! QFile::exists(configFiles[0]) )
	{
		exportDefaultConfig = true;
		exportConfigFileTarget = configFiles[0];
		Warning(log, "Create new config file (%s)",QSTRING_CSTR(configFiles[0]));
	}

	if (exportDefaultConfig)
	{
		Q_INIT_RESOURCE(resource);
		QDir().mkpath(FileUtils::getDirName(exportConfigFileTarget));
		if (QFile::copy(":/hyperion_default.config",exportConfigFileTarget))
		{
			QFile::setPermissions(exportConfigFileTarget, PERM0664 );
			Info(log, "export complete.");
			if (exitAfterExportDefaultConfig) return 0;
		}
		else
		{
			Error(log, "error while export to %s", QSTRING_CSTR(exportConfigFileTarget) );
			return 1;
		}
	}

    int parentPid = parser.value(parentOption).toInt();
	if (parentPid > 0 )
	{
		Info(log, "hyperiond client, parent is pid %d", parentPid);
#ifndef __APPLE__
		prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
	}


	HyperionDaemon* hyperiond = nullptr;
	try
	{
		hyperiond = new HyperionDaemon(configFiles[0], rootPath, qApp, bool(logLevelCheck));
	}
	catch (std::exception& e)
	{
		Error(log, "Hyperion Daemon aborted:\n  %s", e.what());
	}

	int rc = 1;
	try
	{
		// run the application
		if (isGuiApp)
		{
			Info(log, "start systray");
			QApplication::setQuitOnLastWindowClosed(false);
			SysTray tray(hyperiond, hyperiond->getWebServerPort());
			tray.hide();
			rc = (qobject_cast<QApplication *>(app.data()))->exec();
		}
		else
		{
			rc = app->exec();
		}
		Info(log, "Application closed with code %d", rc);
	}
	catch (std::exception& e)
	{
		Error(log, "Hyperion aborted:\n  %s", e.what());
	}

	// delete components
	delete hyperiond;
	Logger::deleteInstance();

	return rc;
}
