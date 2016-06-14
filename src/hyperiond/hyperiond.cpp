// C++ includes
#include <cassert>
#include <csignal>
#include <vector>
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

#ifdef ENABLE_DISPMANX
// Dispmanx grabber includes
#include <grabber/DispmanxWrapper.h>
#endif

#ifdef ENABLE_V4L2
// v4l2 grabber
#include <grabber/V4L2Wrapper.h>
#endif

#ifdef ENABLE_FB
// Framebuffer grabber includes
#include <grabber/FramebufferWrapper.h>
#endif

#ifdef ENABLE_AMLOGIC
#include <grabber/AmlogicWrapper.h>
#endif

#ifdef ENABLE_OSX
// OSX grabber includes
#include <grabber/OsxWrapper.h>
#endif

// XBMC Video checker includes
#include <xbmcvideochecker/XBMCVideoChecker.h>

// Effect engine includes
#include <effectengine/EffectEngine.h>

#ifdef ENABLE_ZEROCONF
#include <bonjour/bonjourserviceregister.h>
#include <bonjour/bonjourrecord.h>
#include <QHostInfo>
#endif

// network servers
#include <jsonserver/JsonServer.h>
#include <protoserver/ProtoServer.h>
#include <boblightserver/BoblightServer.h>
#include <webconfig/WebConfig.h>

#include <sys/prctl.h> 
#include <utils/Logger.h>

using namespace vlofgren;

// ProtoServer includes
#include <protoserver/ProtoServer.h>

// BoblightServer includes
#include <boblightserver/BoblightServer.h>
#include <sys/prctl.h> 

using namespace vlofgren;

void signal_handler(const int signum)
{
	QCoreApplication::quit();

	// reset signal handler to default (in case this handler is not capable of stopping)
	signal(signum, SIG_DFL);
}


Json::Value loadConfig(const std::string & configFile)
{
	// make sure the resources are loaded (they may be left out after static linking)
	Q_INIT_RESOURCE(resource);

	// read the json schema from the resource
	QResource schemaData(":/hyperion-schema");
	assert(schemaData.isValid());

	Json::Reader jsonReader;
	Json::Value schemaJson;
	if (!jsonReader.parse(reinterpret_cast<const char *>(schemaData.data()), reinterpret_cast<const char *>(schemaData.data()) + schemaData.size(), schemaJson, false))
	{
		throw std::runtime_error("ERROR: Json schema wrong: " + jsonReader.getFormattedErrorMessages())	;
	}
	JsonSchemaChecker schemaChecker;
	schemaChecker.setSchema(schemaJson);

	const Json::Value jsonConfig = JsonFactory::readJson(configFile);
	schemaChecker.validate(jsonConfig);

	return jsonConfig;
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


void startBootsequence(const Json::Value &config, Hyperion &hyperion)
{
	// create boot sequence if the configuration is present
	if (config.isMember("bootsequence"))
	{
		const Json::Value effectConfig = config["bootsequence"];

		// Get the parameters for the bootsequence
		const std::string effectName = effectConfig["effect"].asString();
		const unsigned duration_ms   = effectConfig["duration_ms"].asUInt();
		const int priority           = (duration_ms != 0) ? 0 : effectConfig.get("priority",990).asInt();
		const int bootcolor_priority = (priority > 990) ? priority+1 : 990;

		// clear the leds
		ColorRgb boot_color = ColorRgb::BLACK;
		hyperion.setColor(bootcolor_priority, boot_color, 0, false);

		// start boot effect
		if ( ! effectName.empty() )
		{
			int result;
			std::cout << "INFO: Boot sequence '" << effectName << "' ";
			if (effectConfig.isMember("args"))
			{
				std::cout << " (with user defined arguments) ";
				const Json::Value effectConfigArgs = effectConfig["args"];
				result = hyperion.setEffect(effectName, effectConfigArgs, priority, duration_ms);
			}
			else
			{
				result = hyperion.setEffect(effectName, priority, duration_ms);
			}
			std::cout << ((result == 0) ? "started" : "failed") << std::endl;
		}

		// static color
		if ( ! effectConfig["color"].isNull() && effectConfig["color"].isArray() && effectConfig["color"].size() == 3 )
		{
			boot_color = {
				(uint8_t)effectConfig["color"][0].asUInt(),
				(uint8_t)effectConfig["color"][1].asUInt(),
				(uint8_t)effectConfig["color"][2].asUInt()
			};
		}

		hyperion.setColor(bootcolor_priority, boot_color, 0, false);
	}
}


// create XBMC video checker if the configuration is present
void startXBMCVideoChecker(const Json::Value &config, XBMCVideoChecker* &xbmcVideoChecker)
{
	if (config.isMember("xbmcVideoChecker"))
	{
		const Json::Value & videoCheckerConfig = config["xbmcVideoChecker"];
		xbmcVideoChecker = new XBMCVideoChecker(
			videoCheckerConfig["xbmcAddress"].asString(),
			videoCheckerConfig["xbmcTcpPort"].asUInt(),
			videoCheckerConfig["grabVideo"].asBool(),
			videoCheckerConfig["grabPictures"].asBool(),
			videoCheckerConfig["grabAudio"].asBool(),
			videoCheckerConfig["grabMenu"].asBool(),
			videoCheckerConfig.get("grabPause", true).asBool(),
			videoCheckerConfig.get("grabScreensaver", true).asBool(),
			videoCheckerConfig.get("enable3DDetection", true).asBool());

		xbmcVideoChecker->start();
		std::cout << "INFO: Kodi checker created and started" << std::endl;
	}
}

void startNetworkServices(const Json::Value &config, Hyperion &hyperion, JsonServer* &jsonServer, ProtoServer* &protoServer, BoblightServer* &boblightServer, XBMCVideoChecker* &xbmcVideoChecker)
{
	// Create Json server if configuration is present
	unsigned int jsonPort = 19444;
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		//jsonEnable = jsonServerConfig.get("enable", true).asBool();
		jsonPort   = jsonServerConfig.get("port", jsonPort).asUInt();
	}

	jsonServer = new JsonServer(&hyperion, jsonPort );
	std::cout << "INFO: Json server created and started on port " << jsonServer->getPort() << std::endl;

	// Create Proto server if configuration is present
	unsigned int protoPort = 19445;
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		//protoEnable = protoServerConfig.get("enable", true).asBool();
		protoPort  = protoServerConfig.get("port", protoPort).asUInt();
	}

	protoServer = new ProtoServer(&hyperion, protoPort );
	if (xbmcVideoChecker != nullptr)
	{
		QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), protoServer, SIGNAL(grabbingMode(GrabbingMode)));
		QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), protoServer, SIGNAL(videoMode(VideoMode)));
	}
	std::cout << "INFO: Proto server created and started on port " << protoServer->getPort() << std::endl;

#ifdef ENABLE_ZEROCONF
	const Json::Value & deviceConfig = config["device"];
	const std::string deviceName = deviceConfig.get("name", "").asString();

	const std::string hostname = QHostInfo::localHostName().toStdString();
	
	std::string mDNSDescr_json = hostname;
	std::string mDNSService_json = "_hyperiond_json._tcp";
	if (config.isMember("jsonServer"))
	{
		const Json::Value & jsonServerConfig = config["jsonServer"];
		mDNSDescr_json = jsonServerConfig.get("mDNSDescr", mDNSDescr_json).asString();
		mDNSService_json = jsonServerConfig.get("mDNSService", mDNSService_json).asString();
	}
	
	BonjourServiceRegister *bonjourRegister_json = new BonjourServiceRegister();
	bonjourRegister_json->registerService(BonjourRecord((deviceName + " @ " + mDNSDescr_json).c_str(), mDNSService_json.c_str(),
	                                      QString()), jsonServer->getPort() );
	std::cout << "INFO: Json mDNS responder started" << std::endl;

	std::string mDNSDescr_proto = hostname;
	std::string mDNSService_proto = "_hyperiond_proto._tcp";
	if (config.isMember("protoServer"))
	{
		const Json::Value & protoServerConfig = config["protoServer"];
		mDNSDescr_proto = protoServerConfig.get("mDNSDescr", mDNSDescr_proto).asString();
		mDNSService_proto = protoServerConfig.get("mDNSService", mDNSService_proto).asString();
	}
	
	BonjourServiceRegister *bonjourRegister_proto = new BonjourServiceRegister();
	bonjourRegister_proto->registerService(BonjourRecord((deviceName + " @ " + mDNSDescr_proto).c_str(), mDNSService_proto.c_str(),
	                                       QString()), protoServer->getPort() );
	std::cout << "INFO: Proto mDNS responder started" << std::endl;
#endif

	// Create Boblight server if configuration is present
	if (config.isMember("boblightServer"))
	{
		const Json::Value & boblightServerConfig = config["boblightServer"];
		boblightServer = new BoblightServer(&hyperion, boblightServerConfig.get("priority",900).asInt(), boblightServerConfig["port"].asUInt());
		std::cout << "INFO: Boblight server created and started on port " << boblightServer->getPort() << std::endl;
	}
}

#ifdef ENABLE_DISPMANX
void startGrabberDispmanx(const Json::Value &config, Hyperion &hyperion, ProtoServer* &protoServer, XBMCVideoChecker* &xbmcVideoChecker, DispmanxWrapper* &dispmanx)
{
	// Construct and start the frame-grabber if the configuration is present
	if (config.isMember("framegrabber"))
	{
		const Json::Value & frameGrabberConfig = config["framegrabber"];
		dispmanx = new DispmanxWrapper(
			frameGrabberConfig["width"].asUInt(),
			frameGrabberConfig["height"].asUInt(),
			frameGrabberConfig["frequency_Hz"].asUInt(),
			frameGrabberConfig.get("priority",900).asInt(),
			&hyperion);
		dispmanx->setCropping(
					frameGrabberConfig.get("cropLeft", 0).asInt(),
					frameGrabberConfig.get("cropRight", 0).asInt(),
					frameGrabberConfig.get("cropTop", 0).asInt(),
					frameGrabberConfig.get("cropBottom", 0).asInt());

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), dispmanx, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), dispmanx, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(dispmanx, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		dispmanx->start();
		std::cout << "INFO: Frame grabber created and started" << std::endl;
	}
}
#endif

#ifdef ENABLE_V4L2
void startGrabberV4L2(const Json::Value &config, Hyperion &hyperion, ProtoServer* &protoServer, V4L2Wrapper* &v4l2Grabber )
{
	// construct and start the v4l2 grabber if the configuration is present
	if (config.isMember("grabber-v4l2"))
	{
		const Json::Value & grabberConfig = config["grabber-v4l2"];
		v4l2Grabber = new V4L2Wrapper(
					grabberConfig.get("device", "/dev/video0").asString(),
					grabberConfig.get("input", 0).asInt(),
					parseVideoStandard(grabberConfig.get("standard", "no-change").asString()),
					parsePixelFormat(grabberConfig.get("pixelFormat", "no-change").asString()),
					grabberConfig.get("width", -1).asInt(),
					grabberConfig.get("height", -1).asInt(),
					grabberConfig.get("frameDecimation", 2).asInt(),
					grabberConfig.get("sizeDecimation", 8).asInt(),
					grabberConfig.get("redSignalThreshold", 0.0).asDouble(),
					grabberConfig.get("greenSignalThreshold", 0.0).asDouble(),
					grabberConfig.get("blueSignalThreshold", 0.0).asDouble(),
					&hyperion,
					grabberConfig.get("priority", 900).asInt());
		v4l2Grabber->set3D(parse3DMode(grabberConfig.get("mode", "2D").asString()));
		v4l2Grabber->setCropping(
					grabberConfig.get("cropLeft", 0).asInt(),
					grabberConfig.get("cropRight", 0).asInt(),
					grabberConfig.get("cropTop", 0).asInt(),
					grabberConfig.get("cropBottom", 0).asInt());

		QObject::connect(v4l2Grabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		v4l2Grabber->start();
		std::cout << "INFO: V4L2 grabber created and started" << std::endl;
	}

}
#endif

#ifdef ENABLE_AMLOGIC
void startGrabberAmlogic(const Json::Value &config, Hyperion &hyperion, ProtoServer* &protoServer, XBMCVideoChecker* &xbmcVideoChecker, AmlogicWrapper* &amlGrabber)
{
	// Construct and start the framebuffer grabber if the configuration is present
	if (config.isMember("amlgrabber"))
	{
		const Json::Value & grabberConfig = config["amlgrabber"];
		amlGrabber = new AmlogicWrapper(
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt(),
			&hyperion);

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), amlGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)),       amlGrabber, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(amlGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		amlGrabber->start();
		std::cout << "INFO: AMLOGIC grabber created and started" << std::endl;
	}
}
#endif


#ifdef ENABLE_FB
void startGrabberFramebuffer(const Json::Value &config, Hyperion &hyperion, ProtoServer* &protoServer, XBMCVideoChecker* &xbmcVideoChecker, FramebufferWrapper* &fbGrabber)
{
	// Construct and start the framebuffer grabber if the configuration is present
	if (config.isMember("framebuffergrabber") || config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config.isMember("framebuffergrabber")? config["framebuffergrabber"] : config["framegrabber"];
		fbGrabber = new FramebufferWrapper(
			grabberConfig.get("device", "/dev/fb0").asString(),
			grabberConfig["width"].asUInt(),
			grabberConfig["height"].asUInt(),
			grabberConfig["frequency_Hz"].asUInt(),
			grabberConfig.get("priority",900).asInt(),
			&hyperion);

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), fbGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), fbGrabber, SLOT(setVideoMode(VideoMode)));
		}

		QObject::connect(fbGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		fbGrabber->start();
		std::cout << "INFO: Framebuffer grabber created and started" << std::endl;
	}
}
#endif


#ifdef ENABLE_OSX
void startGrabberOsx(const Json::Value &config, Hyperion &hyperion, ProtoServer* &protoServer, XBMCVideoChecker* &xbmcVideoChecker, OsxWrapper* &osxGrabber)
{
	// Construct and start the osx grabber if the configuration is present
	if (config.isMember("osxgrabber") || config.isMember("framegrabber"))
	{
		const Json::Value & grabberConfig = config.isMember("osxgrabber")? config["osxgrabber"] : config["framegrabber"];
		osxGrabber = new OsxWrapper(
									grabberConfig.get("display", 0).asUInt(),
									grabberConfig["width"].asUInt(),
									grabberConfig["height"].asUInt(),
									grabberConfig["frequency_Hz"].asUInt(),
									grabberConfig.get("priority",900).asInt(),
									&hyperion );

		if (xbmcVideoChecker != nullptr)
		{
			QObject::connect(xbmcVideoChecker, SIGNAL(grabbingMode(GrabbingMode)), osxGrabber, SLOT(setGrabbingMode(GrabbingMode)));
			QObject::connect(xbmcVideoChecker, SIGNAL(videoMode(VideoMode)), osxGrabber, SLOT(setVideoMode(VideoMode)));
		}
		
		QObject::connect(osxGrabber, SIGNAL(emitImage(int, const Image<ColorRgb>&, const int)), protoServer, SLOT(sendImageToProtoSlaves(int, const Image<ColorRgb>&, const int)) );

		osxGrabber->start();
		std::cout << "INFO: OSX grabber created and started" << std::endl;
	}
}
#endif

int main(int argc, char** argv)
{
	std::cout
		<< "Hyperion Ambilight Deamon (" << getpid() << ")" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION_ID << std::endl
		<< "\tBuild Time: " << __DATE__ << " " << __TIME__ << std::endl;

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

	if (configFiles.size() == 0)
	{
		std::cout << "ERROR: Missing required configuration file. Usage:" << std::endl;
		std::cout << "hyperiond <options ...> [config.file ...]" << std::endl;
		return 1;
	}

	
	if (argParentPid.getValue() > 0 )
	{
		std::cout << "hyperiond client, parent is pid " << argParentPid.getValue() << std::endl;
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
		std::cout << "ERROR: No valid config found " << std::endl;
		return 1;
	}
	
	const std::string configFile = configFiles[argvId];
	std::cout << "INFO: Selected configuration file: " << configFile.c_str() << std::endl;
	const Json::Value config = loadConfig(configFile);

	Hyperion hyperion(config, configFile);
	std::cout << "INFO: Hyperion started and initialised" << std::endl;


	startBootsequence(config, hyperion);

	XBMCVideoChecker * xbmcVideoChecker = nullptr;
	startXBMCVideoChecker(config, xbmcVideoChecker);

	// ---- network services -----
	JsonServer * jsonServer = nullptr;
	ProtoServer * protoServer = nullptr;
	BoblightServer * boblightServer = nullptr;
	startNetworkServices(config, hyperion, jsonServer, protoServer, boblightServer, xbmcVideoChecker);

#ifdef ENABLE_QT5
	WebConfig webConfig(&hyperion, &app);
#endif

// ---- grabber -----

#ifdef ENABLE_DISPMANX
	DispmanxWrapper * dispmanx = nullptr;
	startGrabberDispmanx(config, hyperion, protoServer, xbmcVideoChecker, dispmanx);
#else
#if !defined(ENABLE_OSX) && !defined(ENABLE_FB)
	if (config.isMember("framegrabber"))
	{
		std::cerr << "ERRROR: The dispmanx framegrabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif
#endif

#ifdef ENABLE_V4L2
	V4L2Wrapper * v4l2Grabber = nullptr;
	startGrabberV4L2(config, hyperion, protoServer, v4l2Grabber);
#else
	if (config.isMember("grabber-v4l2"))
	{
		std::cerr << "ERROR: The v4l2 grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif

#ifdef ENABLE_AMLOGIC
	// Construct and start the framebuffer grabber if the configuration is present
	AmlogicWrapper * amlGrabber = nullptr;
	startGrabberAmlogic(config, hyperion, protoServer, xbmcVideoChecker, amlGrabber);
#else
	if (config.isMember("amlgrabber"))
	{
		std::cerr << "ERROR: The AMLOGIC grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif

#ifdef ENABLE_FB
	// Construct and start the framebuffer grabber if the configuration is present
	FramebufferWrapper * fbGrabber = nullptr;
	startGrabberFramebuffer(config, hyperion, protoServer, xbmcVideoChecker, fbGrabber);
#else
	if (config.isMember("framebuffergrabber"))
	{
		std::cerr << "ERROR: The framebuffer grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_OSX)
	else if (config.isMember("framegrabber"))
	{
		std::cerr << "ERROR: The framebuffer grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif
#endif

#ifdef ENABLE_OSX
	// Construct and start the osx grabber if the configuration is present
	OsxWrapper * osxGrabber = nullptr;
	startGrabberDispmanx(config, hyperion, protoServer, xbmcVideoChecker, osxGrabber);
#else
	if (config.isMember("osxgrabber"))
	{
		std::cerr << "ERROR: The osx grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#if !defined(ENABLE_DISPMANX) && !defined(ENABLE_FB)
	else if (config.isMember("framegrabber"))
	{
		std::cerr << "ERROR: The osx grabber can not be instantiated, because it has been left out from the build" << std::endl;
	}
#endif
#endif


	// run the application
	int rc = app.exec();
	std::cout << "INFO: Application closed with code " << rc << std::endl;

	// Delete all component
#ifdef ENABLE_DISPMANX
	delete dispmanx;
#endif
#ifdef ENABLE_FB
	delete fbGrabber;
#endif
#ifdef ENABLE_OSX
	delete osxGrabber;
#endif
#ifdef ENABLE_V4L2
	delete v4l2Grabber;
#endif
	delete xbmcVideoChecker;
	delete jsonServer;
	delete protoServer;
	delete boblightServer;

	// leave application
	return rc;
}
