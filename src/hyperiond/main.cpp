#include <cassert>
#include <csignal>
#include <unistd.h>

#ifndef __APPLE__
/* prctl is Linux only */
#include <sys/prctl.h>
#endif

#include <exception>

#include <QCoreApplication>
#include <QLocale>
#include <QFile>
#include <QString>

#include "HyperionConfig.h"

#include <utils/Logger.h>
#include <webconfig/WebConfig.h>
#include <commandline/Parser.h>
#include <commandline/IntOption.h>

#include "hyperiond.h"

using namespace commandline;

void signal_handler(const int signum)
{
	QCoreApplication::quit();

	// reset signal handler to default (in case this handler is not capable of stopping)
	signal(signum, SIG_DFL);
}


void startNewHyperion(int parentPid, std::string hyperionFile, std::string configFile)
{
	if ( fork() == 0 )
	{
		sleep(3);
		execl(hyperionFile.c_str(), hyperionFile.c_str(), "--parent", QString::number(parentPid).toStdString().c_str(), configFile.c_str(), NULL);
		exit(0);
	}
}


int main(int argc, char** argv)
{
	// initialize main logger and set global log level
	Logger* log = Logger::getInstance("MAIN");
	Logger::setLogLevel(Logger::WARNING);

	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGCHLD, signal_handler);
	signal(SIGPIPE, signal_handler);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	Parser parser("Hyperion Daemon");
	parser.addHelpOption();

	Option versionOption("version");
	versionOption.setDescription(QCoreApplication::translate("main", "Show version information"));
	versionOption.setDefaultValue("0");
	parser.addOption(versionOption);

	IntOption & parentOption = parser.add<IntOption>('p', "parent", "pid of parent hyperiond"); // 2^22 is the max for Linux

	Option silentOption(QStringList() << "s" << "silent");
    silentOption.setDescription(QCoreApplication::translate("main", "do not print any outputs"));
	parser.addOption(silentOption);

	Option verboseOption(QStringList() << "v" << "verbose");
    verboseOption.setDescription(QCoreApplication::translate("main", "Increase verbosity"));
	parser.addOption(verboseOption);

	Option debugOption(QStringList() << "d" << "debug");
    debugOption.setDescription(QCoreApplication::translate("main", "Show debug messages"));
	parser.addOption(debugOption);

	parser.addPositionalArgument("config-files", QCoreApplication::translate("main", "Configuration files"), "[files...]");

    parser.process(app);

	const QStringList configFiles = parser.positionalArguments();

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

	if (configFiles.size() == 0)
	{
		Error(log, "Missing required configuration file. Usage: hyperiond <options ...> [config.file ...]");
		return 1;
	}


    int parentPid = parser.value(parentOption).toInt();
	if (parentPid > 0 )
	{
		Info(log, "hyperiond client, parent is pid %d", parentPid);
#ifndef __APPLE__
		prctl(PR_SET_PDEATHSIG, SIGHUP);
#endif
	}

	int argvId = -1;
	for(size_t idx=0; idx < configFiles.size(); idx++) {
		if ( QFile::exists(configFiles[idx]))
		{
			if (argvId < 0) argvId=idx;
			else startNewHyperion(getpid(), argv[0], configFiles[idx].toStdString());
		}
	}

	if ( argvId < 0)
	{
		Error(log, "No valid config found");
		return 1;
	}

	HyperionDaemon* hyperiond = nullptr;
	try
	{
		hyperiond = new HyperionDaemon(configFiles[argvId], &app);
		hyperiond->run();
	}
	catch (std::exception& e)
	{
		Error(log, "Hyperion Daemon aborted:\n  %s", e.what());
	}

	int rc = 1;
	WebConfig* webConfig = nullptr;
	try
	{
		webConfig = new WebConfig(&app);
		// run the application
		rc = app.exec();
		Info(log, "INFO: Application closed with code %d", rc);
	}
	catch (std::exception& e)
	{
		Error(log, "Hyperion aborted:\n  %s", e.what());
	}

	// delete components
	delete webConfig;
	delete hyperiond;
	Logger::deleteInstance();

	return rc;
}
