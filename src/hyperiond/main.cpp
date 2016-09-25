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
#include <QResource>
#include <QDir>
#include <QStringList>

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
	//Logger::setLogLevel(Logger::WARNING);
	Logger::setLogLevel(Logger::DEBUG, "LedDevice");

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

	BooleanOption & versionOption       = parser.add<BooleanOption>(0x0, "version", "Show version information");
	IntOption     &  parentOption       = parser.add<IntOption>    ('p', "parent", "pid of parent hyperiond"); // 2^22 is the max for Linux
	BooleanOption &  silentOption       = parser.add<BooleanOption>('s', "silent", "do not print any outputs");
	BooleanOption & verboseOption       = parser.add<BooleanOption>('v', "verbose", "Increase verbosity");
	BooleanOption &   debugOption       = parser.add<BooleanOption>('d', "debug", "Show debug messages");
	Option        & exportConfigOption  = parser.add<Option>       (0x0, "export-config", "export default config to file");
	Option        & exportEfxOption     = parser.add<Option>       (0x0, "export-effects", "export effects to given path");

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

	if (parser.isSet(exportConfigOption))
	{
		Q_INIT_RESOURCE(resource);
		if (QFile::copy(":/hyperion_default.config",exportConfigOption.value(parser)))
		{
			Info(log, "export complete.");
			return 0;
		}
		Error(log, "can not export to %s",exportConfigOption.getCString(parser));
		return 1;
	}
	
	if (parser.isSet(exportEfxOption))
	{
		Q_INIT_RESOURCE(EffectEngine);
		QDir directory(":/effects/");
		QDir destDir(exportEfxOption.value(parser));
		if (directory.exists() && destDir.exists())
		{
			std::cout << "extract to folder: " << std::endl;
			QStringList filenames = directory.entryList(QStringList() << "*.json", QDir::Files, QDir::Name | QDir::IgnoreCase);
			foreach (const QString & filename, filenames)
			{
				if (QFile::exists(destDir.dirName()+"/"+filename))
				{
					QFile::remove(destDir.dirName()+"/"+filename);
				}
				
				std::cout << "Extract: " << filename.toStdString() << " ... ";
				if (QFile::copy(QString(":/effects/")+filename, destDir.dirName()+"/"+filename))
				{
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
	for(int idx=0; idx < configFiles.size(); idx++) {
		if ( QFile::exists(configFiles[idx]))
		{
			if (argvId < 0) argvId=idx;
			else startNewHyperion(getpid(), argv[0], configFiles[idx].toStdString());
		}
	}

	if ( argvId < 0)
	{
		Warning(log, "No valid config found");
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
