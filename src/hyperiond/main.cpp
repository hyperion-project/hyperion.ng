#include <cassert>
#include <csignal>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

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
#include <QProcess>

#include "HyperionConfig.h"

#include <utils/Logger.h>
#include <utils/FileUtils.h>
#include <commandline/Parser.h>
#include <commandline/IntOption.h>
#include <../../include/db/AuthTable.h>

#ifdef ENABLE_X11
#include <X11/Xlib.h>
#endif

#include "hyperiond.h"
#include "systray.h"

using namespace commandline;

#define PERM0664 QFileDevice::ReadOwner | QFileDevice::ReadGroup | QFileDevice::ReadOther | QFileDevice::WriteOwner | QFileDevice::WriteGroup

unsigned int getProcessIdsByProcessName(const char* processName, QStringList &listOfPids)
{
	// Clear content of returned list of PIDS
	listOfPids.clear();

#if defined(WIN32)
	// Get the list of process identifiers.
	DWORD aProcesses[1024], cbNeeded, cProcesses;
	unsigned int i;

	if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded))
		return 0;

	// Calculate how many process identifiers were returned.
	cProcesses = cbNeeded / sizeof(DWORD);

	// Search for a matching name for each process
	for (i = 0; i < cProcesses; i++)
	{
		if (aProcesses[i] != 0)
		{
			char szProcessName[MAX_PATH] = {0};

			DWORD processID = aProcesses[i];

			// Get a handle to the process.
			HANDLE hProcess = OpenProcess( PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);

			// Get the process name
			if (NULL != hProcess)
			{
				HMODULE hMod;
				DWORD cbNeeded;

				if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded))
					GetModuleBaseNameA(hProcess, hMod, szProcessName, sizeof(szProcessName)/sizeof(char));

				// Release the handle to the process.
				CloseHandle(hProcess);

				if (*szProcessName != 0 && strcmp(processName, szProcessName) == 0)
					listOfPids.append(QString::number(processID));
			}
		}
	}

	return listOfPids.count();

#else

	// Run pgrep, which looks through the currently running processses and lists the process IDs
	// which match the selection criteria to stdout.
	QProcess process;
	process.start("pgrep",  QStringList() << processName);
	process.waitForReadyRead();

	QByteArray bytes = process.readAllStandardOutput();

	process.terminate();
	process.waitForFinished();
	process.kill();

	// Output is something like "2472\n2323" for multiple instances
	if (bytes.isEmpty())
		return 0;

	listOfPids = QString(bytes).split("\n", QString::SkipEmptyParts);
	return listOfPids.count();

#endif
}

void signal_handler(const int signum)
{
//	SIGUSR1 and SIGUSR2 must be rewritten

	// Hyperion Managment instance
	HyperionIManager* _hyperion = HyperionIManager::getInstance();

	if(signum == SIGCHLD)
	{
		// only quit when a registered child process is gone
		// currently this feature is not active ...
		return;
	}
	else if (signum == SIGUSR1)
	{
		if (_hyperion != nullptr)
		{
// 			_hyperion->setComponentState(hyperion::COMP_SMOOTHING, false);
// 			_hyperion->setComponentState(hyperion::COMP_LEDDEVICE, false);
		}
		return;
	}
	else if (signum == SIGUSR2)
	{
		if (_hyperion != nullptr)
		{
// 			_hyperion->setComponentState(hyperion::COMP_LEDDEVICE, true);
// 			_hyperion->setComponentState(hyperion::COMP_SMOOTHING, true);
		}
		return;
	}

	QCoreApplication::quit();

	// reset signal handler to default (in case this handler is not capable of stopping)
	signal(signum, SIG_DFL);
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
		app->setWindowIcon(QIcon(":/hyperion-icon-32px.png"));
		return app;
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

	// check if we are running already an instance
	// TODO Do not use pgrep on linux, instead iter /proc
	// TODO Allow one session per user
	// http://www.qtcentre.org/threads/44489-Get-Process-ID-for-a-running-application
	QStringList listOfPids;
	if(getProcessIdsByProcessName("hyperiond", listOfPids) > 1)
	{
		Error(log, "The Hyperion Daemon is already running, abort start");
		return 0;
	}

	// Initialising QCoreApplication
	QScopedPointer<QCoreApplication> app(createApplication(argc, argv));

	bool isGuiApp = (qobject_cast<QApplication *>(app.data()) != 0 && QSystemTrayIcon::isSystemTrayAvailable());

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGABRT, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGPIPE, signal_handler);
	signal(SIGUSR1, signal_handler);
	signal(SIGUSR2, signal_handler);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	Parser parser("Hyperion Daemon");
	parser.addHelpOption();

	BooleanOption & versionOption       = parser.add<BooleanOption> (0x0, "version", "Show version information");
	Option        & userDataOption      = parser.add<Option>        ('u', "userdata", "Overwrite user data path, defaults to home directory of current user (%1)", QDir::homePath() + "/.hyperion");
	BooleanOption & resetPassword       = parser.add<BooleanOption> (0x0, "resetPassword", "Lost your password? Reset it with this option back to 'hyperion'");
	BooleanOption & silentOption        = parser.add<BooleanOption> ('s', "silent", "do not print any outputs");
	BooleanOption & verboseOption       = parser.add<BooleanOption> ('v', "verbose", "Increase verbosity");
	BooleanOption & debugOption         = parser.add<BooleanOption> ('d', "debug", "Show debug messages");
                                          parser.add<BooleanOption> (0x0, "desktop", "show systray on desktop");
	                                      parser.add<BooleanOption> (0x0, "service", "force hyperion to start as console service");
	Option        & exportEfxOption     = parser.add<Option>        (0x0, "export-effects", "export effects to given path");

	parser.process(*qApp);

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
