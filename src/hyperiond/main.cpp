// C++ includes
#include <cassert>
#include <csignal>
#include <unistd.h>

// QT includes
#include <QCoreApplication>
#include <QResource>
#include <QLocale>
#include <QFile>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

// config includes
#include "HyperionConfig.h"

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>


// network servers
#include <webconfig/WebConfig.h>

#include <sys/prctl.h> 
#include <utils/Logger.h>

#include "hyperiond.h"


using namespace vlofgren;

int main(int argc, char** argv)
{
	Logger* log = Logger::getInstance("MAIN", Logger::DEBUG);

	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);
	signal(SIGCHLD, signal_handler);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	OptionsParser optionParser("Hyperion Daemon");
	ParameterSet & parameters = optionParser.getParameters();

	SwitchParameter<>      & argVersion               = parameters.add<SwitchParameter<>>     (0x0, "version",       "Show version information");
	IntParameter           & argParentPid             = parameters.add<IntParameter>          (0x0, "parent",        "pid of parent hyperiond");
	SwitchParameter<>      & argHelp                  = parameters.add<SwitchParameter<>>     ('h', "help",          "Show this help message and exit");

	argParentPid.setDefault(0);
	optionParser.parse(argc, const_cast<const char **>(argv));
	const std::vector<std::string> configFiles = optionParser.getFiles();

	// check if we need to display the usage. exit if we do.
	if (argHelp.isSet())
	{
		optionParser.usage();
		return 0;
	}

	if (argVersion.isSet())
	{
	std::cout
		<< "Hyperion Ambilight Deamon (" << getpid() << ")" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION_ID << std::endl
		<< "\tBuild Time: " << __DATE__ << " " << __TIME__ << std::endl;

		return 0;
	}

	if (configFiles.size() == 0)
	{
		Error(log, "Missing required configuration file. Usage: hyperiond <options ...> [config.file ...]");
		return 1;
	}

	
	if (argParentPid.getValue() > 0 )
	{
		Info(log, "hyperiond client, parent is pid %d",argParentPid.getValue());
		prctl(PR_SET_PDEATHSIG, SIGHUP);
	}
	
	int argvId = -1;
	for(size_t idx=0; idx < configFiles.size(); idx++) {
		if ( QFile::exists(configFiles[idx].c_str()))
		{
			if (argvId < 0) argvId=idx;
			else startNewHyperion(getpid(), argv[0], configFiles[idx].c_str());
		}
	}
	
	if ( argvId < 0)
	{
		Error(log, "No valid config found");
		return 1;
	}
	
	const std::string configFile = configFiles[argvId];
	Info(log, "Selected configuration file: %s", configFile.c_str() );
	const Json::Value config = loadConfig(configFile);

	Hyperion::initInstance(config, configFile);
	Info(log, "Hyperion started and initialised");


	startBootsequence();

	XBMCVideoChecker * xbmcVideoChecker = createXBMCVideoChecker();

	// ---- network services -----
	JsonServer * jsonServer = nullptr;
	ProtoServer * protoServer = nullptr;
	BoblightServer * boblightServer = nullptr;
	startNetworkServices(jsonServer, protoServer, boblightServer, xbmcVideoChecker);

	#ifdef ENABLE_QT5
		WebConfig webConfig(&app);
	#endif

	// ---- grabber -----
	// if a grabber is left out of build, then <grabber>Wrapper is set to QObject as dummy and has value nullptr
	V4L2Wrapper * v4l2Grabber = createGrabberV4L2(protoServer);
	#ifndef ENABLE_V4L2
		ErrorIf(config.isMember("grabber-v4l2"), log, "The v4l2 grabber can not be instantiated, because it has been left out from the build");
	#endif


	DispmanxWrapper * dispmanx = createGrabberDispmanx(protoServer, xbmcVideoChecker);
	#ifndef ENABLE_DISPMANX
		ErrorIf(config.isMember("framegrabber"), log, "The dispmanx framegrabber can not be instantiated, because it has been left out from the build");
	#endif

	AmlogicWrapper * amlGrabber = createGrabberAmlogic(protoServer, xbmcVideoChecker );
	#ifndef ENABLE_AMLOGIC
		ErrorIf(config.isMember("amlgrabber"), log, "The AMLOGIC grabber can not be instantiated, because it has been left out from the build");
	#endif

	FramebufferWrapper * fbGrabber = createGrabberFramebuffer(protoServer, xbmcVideoChecker);
	#ifndef ENABLE_FB
		ErrorIf(config.isMember("framebuffergrabber"), log, "The framebuffer grabber can not be instantiated, because it has been left out from the build");
	#endif

	OsxWrapper * osxGrabber = createGrabberDispmanx(protoServer, xbmcVideoChecker);
	#ifndef ENABLE_OSX
		ErrorIf(config.isMember("osxgrabber"), log, "The osx grabber can not be instantiated, because it has been left out from the build");
	#endif

	#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX) && !defined(ENABLE_FB)
		ErrorIf(config.isMember("framegrabber"), log, "No grabber can be instantiated, because all grabbers have been left out from the build" << std::endl;
	#endif

	// run the application
	int rc = app.exec();
	Info(log, "INFO: Application closed with code %d", rc);

	// Delete all component
	delete amlGrabber;
	delete dispmanx;
	delete fbGrabber;
	delete osxGrabber;
	delete v4l2Grabber;
	delete xbmcVideoChecker;
	delete jsonServer;
	delete protoServer;
	delete boblightServer;

	return rc;
}
