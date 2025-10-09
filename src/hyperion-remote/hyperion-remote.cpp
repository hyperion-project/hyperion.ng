// stl includes
#include <clocale>
#include <initializer_list>
#include <iostream>
#include <stdlib.h>

// Qt includes
#include <QCoreApplication>
#include <QTimer>
#include <QHostAddress>
#include <QLocale>
#include <QTimer>

#include <utils/DefaultSignalHandler.h>
#include <utils/ErrorManager.h>
#include <utils/Logger.h>

#include "HyperionConfig.h"
#include <commandline/Parser.h>

#ifdef ENABLE_MDNS
// mDNS discover
#include <mdns/MdnsBrowser.h>
#include <mdns/MdnsServiceRegister.h>
#else
// ssdp discover
#include <ssdp/SSDPDiscover.h>
#endif
#include <utils/NetUtils.h>

// hyperion-remote include
#include "JsonConnection.h"

using namespace commandline;

ErrorManager errorManager;

/// Count the number of true values in a list of booleans
int count(std::initializer_list<bool> values)
{
	return std::count_if(values.begin(), values.end(), [](bool value) { return value; });
}

void showHelp(Option & option){
	QString shortOption;
	QString const longOption = QString("--%1").arg(option.names().constLast());

	if(option.names().size() == 2){
		shortOption = QString("-%1").arg(option.names().constFirst());
	}

	qWarning() << qPrintable(QString("\t%1\t%2\t%3").arg(shortOption, longOption, option.description()));
}

int getRunningInstance(const QJsonObject &reply, const QString &name = "") {
	if (reply.contains("instance")) {
		const QJsonArray list = reply.value("instance").toArray();

		for (const auto &entry : list) {
			const QJsonObject obj = entry.toObject();

			// Check if the instance is running
			if (obj["running"].toBool()) {
				// If name is empty, return the first running instance
				if (name.isEmpty() || obj["friendly_name"] == name) {
					return obj["instance"].toInt();
				}
			}
		}
	}

	emit errorManager.errorOccurred(
				name.isEmpty()
				? "No running instance found at this Hyperion server."
				: QString("Cannot find a running instance with name (%1) at this Hyperion server.").arg(name)
				  );

	return -1;  // Return -1 if no matching running instance is found
}

int main(int argc, char * argv[])
{
	DefaultSignalHandler::install();

	Logger* log = Logger::getInstance("REMOTE");
	Logger::setLogLevel(Logger::INFO);

	QCoreApplication const app(argc, argv);

	QString const baseName = QCoreApplication::applicationName();
	std::cout << baseName.toStdString() << ":\n"
			  << "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")\n"
			  << "\tbuild time: " << __DATE__ << " " << __TIME__ << "\n";

	QObject::connect(&errorManager, &ErrorManager::errorOccurred, [&](const QString& error) {
		Error(log, "Error occured: %s", QSTRING_CSTR(error));
		QTimer::singleShot(0, [&app]() { app.quit(); });
	});

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	// create the option parser and initialize all parameters
	Parser parser("Application to send a command to hyperion using the JSON interface");

	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//      art             variable definition       append art to Parser     short-, long option              description, optional default value      //
	///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Option          & argAddress            = parser.add<Option>       ('a', "address"                , "The hostname or IP-address (IPv4 or IPv6) of the hyperion server.\nDefault port: 19444.\nSample addresses:\nHost : hyperion.fritz.box\nIPv4 : 127.0.0.1:19444\nIPv6 : [2001:1:2:3:4:5:6:7]", "127.0.0.1");
	Option          & argToken              = parser.add<Option>       ('t', "token"                  , "If authorization tokens are required, this token is used");
	Option          & argInstance           = parser.add<Option>       ('I', "instance"               , "Select a specific target instance by name for your command. By default it uses always the first instance");
	IntOption       & argPriority           = parser.add<IntOption>    ('p', "priority"               , "Used to the provided priority channel (suggested 2-99) [default: %1]", "50");
	IntOption       & argDuration           = parser.add<IntOption>    ('d', "duration"               , "Specify how long the LEDs should be switched on in milliseconds [default: infinity]");
	ColorsOption    & argColor              = parser.add<ColorsOption> ('c', "color"                  , "Set all LEDs to a constant color (either via an RRGGBB hex value string or a color name. The color may be repeated multiple time like: RRGGBBRRGGBB)");
	ImageOption     & argImage              = parser.add<ImageOption>  ('i', "image"                  , "Set the LEDs to the colors according to the given image file");
#if defined(ENABLE_EFFECTENGINE)
	Option          & argEffect             = parser.add<Option>       ('e', "effect"                 , "Enable the effect with the given name");
	Option          & argEffectFile         = parser.add<Option>       (0x0, "effectFile"             , "Arguments to use in combination with --createEffect");
	Option          & argEffectArgs         = parser.add<Option>       (0x0, "effectArgs"             , "Arguments to use in combination with the specified effect. Should be a JSON object string.", "");
	Option          & argCreateEffect       = parser.add<Option>       (0x0, "createEffect"           , "Write a new JSON Effect configuration file.\nFirst parameter = Effect name.\nSecond parameter = Effect file (--effectFile).\nLast parameter = Effect arguments (--effectArgs.)", "");
	Option          & argDeleteEffect       = parser.add<Option>       (0x0, "deleteEffect"           , "Delete a custom created JSON Effect configuration file.");
#endif
	BooleanOption   & argClear              = parser.add<BooleanOption>('x', "clear"                  , "Clear data for the priority channel provided by the -p option");
	BooleanOption   & argClearAll           = parser.add<BooleanOption>(0x0, "clearall"               , "Clear data for all active priority channels");
	Option          & argEnableComponent    = parser.add<Option>       ('E', "enable"                 , "Enable the Component with the given name. Available Components are [SMOOTHING, BLACKBORDER, FORWARDER, BOBLIGHTSERVER, GRABBER, V4L, AUDIO, LEDDEVICE]");
	Option          & argDisableComponent   = parser.add<Option>       ('D', "disable"                , "Disable the Component with the given name. Available Components are [SMOOTHING, BLACKBORDER, FORWARDER, BOBLIGHTSERVER, GRABBER, V4L, AUDIO, LEDDEVICE]");
	Option          & argId                 = parser.add<Option>       ('q', "qualifier"              , "Identifier(qualifier) of the adjustment to set");
	IntOption       & argBrightness         = parser.add<IntOption>    ('L', "brightness"             , "Set the brightness gain of the LEDs");
	IntOption       & argBrightnessC        = parser.add<IntOption>    (0x0, "brightnessCompensation" , "Set the brightness compensation");
	IntOption       & argBacklightThreshold = parser.add<IntOption>    ('n', "backlightThreshold"     , "threshold for activating backlight (minimum brightness)");
	IntOption       & argBacklightColored   = parser.add<IntOption>    (0x0, "backlightColored"       , "0 = white backlight; 1 =  colored backlight");
	DoubleOption    & argGamma              = parser.add<DoubleOption> ('g', "gamma"                  , "Set the overall gamma of the LEDs");
	ColorOption     & argRAdjust            = parser.add<ColorOption>  ('R', "redAdjustment"          , "Set the adjustment of the red color (requires colors in hex format as RRGGBB)");
	ColorOption     & argGAdjust            = parser.add<ColorOption>  ('G', "greenAdjustment"        , "Set the adjustment of the green color (requires colors in hex format as RRGGBB)");
	ColorOption     & argBAdjust            = parser.add<ColorOption>  ('B', "blueAdjustment"         , "Set the adjustment of the blue color (requires colors in hex format as RRGGBB)");
	ColorOption     & argCAdjust            = parser.add<ColorOption>  ('C', "cyanAdjustment"         , "Set the adjustment of the cyan color (requires colors in hex format as RRGGBB)");
	ColorOption     & argMAdjust            = parser.add<ColorOption>  ('M', "magentaAdjustment"      , "Set the adjustment of the magenta color (requires colors in hex format as RRGGBB)");
	ColorOption     & argYAdjust            = parser.add<ColorOption>  ('Y', "yellowAdjustment"       , "Set the adjustment of the yellow color (requires colors in hex format as RRGGBB)");
	ColorOption     & argWAdjust            = parser.add<ColorOption>  ('W', "whiteAdjustment"        , "Set the adjustment of the white color (requires colors in hex format as RRGGBB)");
	ColorOption     & argbAdjust            = parser.add<ColorOption>  ('b', "blackAdjustment"        , "Set the adjustment of the black color (requires colors in hex format as RRGGBB)");
	Option          & argMapping            = parser.add<Option>       ('m', "ledMapping"             , "Set the method for image to led mapping valid values: multi color_mean, uni-color_mean");
	Option          & argVideoMode          = parser.add<Option>       ('V', "videoMode"              , "Set the video mode valid values: 2D, 3DSBS, 3DTAB");
	IntOption       & argSource             = parser.add<IntOption>    (0x0, "sourceSelect"           , "Set current active priority channel and deactivate auto source switching");
	BooleanOption   & argSourceAuto         = parser.add<BooleanOption>(0x0, "sourceAutoSelect"       , "Enables auto source, if disabled prio by manual selecting input source");
	BooleanOption   & argOff                = parser.add<BooleanOption>(0x0, "off"                    , "Deactivates hyperion");
	BooleanOption   & argOn                 = parser.add<BooleanOption>(0x0, "on"                     , "Activates hyperion");
	BooleanOption   & argConfigGet          = parser.add<BooleanOption>(0x0, "configGet"              , "Print the current loaded Hyperion configuration file");
	BooleanOption   & argSchemaGet          = parser.add<BooleanOption>(0x0, "schemaGet"              , "Print the JSON schema for Hyperion configuration");
	Option          & argConfigSet          = parser.add<Option>       (0x0, "configSet"              , "Write to the actual loaded configuration file. Should be a JSON object string.");
	BooleanOption   & argServerInfo         = parser.add<BooleanOption>('l', "list"                   , "List server info and active effects with priority and duration");
	BooleanOption   & argSysInfo            = parser.add<BooleanOption>('s', "sysinfo"                , "Show system info");
	BooleanOption   & argSystemSuspend      = parser.add<BooleanOption>(0x0, "suspend"                , "Suspend Hyperion. Stop all instances and components");
	BooleanOption   & argSystemResume       = parser.add<BooleanOption>(0x0, "resume"                 , "Resume Hyperion. Start all instances and components");
	BooleanOption   & argSystemToggleSuspend= parser.add<BooleanOption>(0x0, "toggleSuspend"          , "Toggle between Suspend and Resume. First request will trigger suspend");
	BooleanOption   & argSystemIdle         = parser.add<BooleanOption>(0x0, "idle"                   , "Put Hyperion in Idle mode, i.e. all instances, components will be disabled besides the output processing (LED-devices, smoothing).");
	BooleanOption   & argSystemToggleIdle   = parser.add<BooleanOption>(0x0, "toggleIdle"             , "Toggle between Idle and Working mode. First request will trigger Idle mode");
	BooleanOption   & argSystemRestart      = parser.add<BooleanOption>(0x0, "restart"                , "Restart Hyperion");

	BooleanOption   & argPrint              = parser.add<BooleanOption>(0x0, "print", "Print the JSON input and output messages on stdout");
	BooleanOption   & argDebug              = parser.add<BooleanOption>(0x0, "debug", "Enable debug logging");
	BooleanOption   & argHelp               = parser.add<BooleanOption>('h', "help", "Show this help message and exit");

	// parse all _options
	parser.process(app);

	// check if debug logging is required
	if (parser.isSet(argDebug))
	{
		Logger::setLogLevel(Logger::DEBUG);
	}

	// check if we need to display the usage. exit if we do.
	if (parser.isSet(argHelp))
	{
		parser.showHelp(0);
	}

	// check if at least one of the available color transforms is set
	bool colorAdjust = parser.isSet(argRAdjust) || parser.isSet(argGAdjust) || parser.isSet(argBAdjust) || parser.isSet(argCAdjust) || parser.isSet(argMAdjust)
					   || parser.isSet(argYAdjust) || parser.isSet(argWAdjust) || parser.isSet(argbAdjust) || parser.isSet(argGamma)|| parser.isSet(argBrightness)|| parser.isSet(argBrightnessC)
					   || parser.isSet(argBacklightThreshold) || parser.isSet(argBacklightColored);

	// check that exactly one command was given
	int const commandCount = count({ parser.isSet(argColor), parser.isSet(argImage),
								 #if defined(ENABLE_EFFECTENGINE)
									 parser.isSet(argEffect), parser.isSet(argCreateEffect), parser.isSet(argDeleteEffect),
								 #endif
									 parser.isSet(argServerInfo), parser.isSet(argSysInfo),
									 parser.isSet(argSystemSuspend), parser.isSet(argSystemResume), parser.isSet(argSystemToggleSuspend),
									 parser.isSet(argSystemIdle), parser.isSet(argSystemToggleIdle),
									 parser.isSet(argSystemRestart),
									 parser.isSet(argClear), parser.isSet(argClearAll), parser.isSet(argEnableComponent), parser.isSet(argDisableComponent), colorAdjust,
									 parser.isSet(argSource), parser.isSet(argSourceAuto), parser.isSet(argOff), parser.isSet(argOn), parser.isSet(argConfigGet), parser.isSet(argSchemaGet), parser.isSet(argConfigSet),
									 parser.isSet(argMapping),parser.isSet(argVideoMode) });
	if (commandCount != 1)
	{
		qWarning() << (commandCount == 0 ? "No command found." : "Multiple commands found.") << " Provide exactly one of the following options:";
		showHelp(argColor);
		showHelp(argImage);
#if defined(ENABLE_EFFECTENGINE)
		showHelp(argEffect);
		showHelp(argCreateEffect);
		showHelp(argDeleteEffect);
#endif
		showHelp(argServerInfo);
		showHelp(argSysInfo);
		showHelp(argSystemSuspend);
		showHelp(argSystemResume);
		showHelp(argSystemToggleSuspend);
		showHelp(argSystemIdle);
		showHelp(argSystemToggleIdle);
		showHelp(argSystemRestart);
		showHelp(argClear);
		showHelp(argClearAll);
		showHelp(argEnableComponent);
		showHelp(argDisableComponent);
		showHelp(argSource);
		showHelp(argSourceAuto);
		showHelp(argConfigGet);
		showHelp(argVideoMode);
		qWarning() << "or one or more of the available color modding operations:";
		showHelp(argId);
		showHelp(argBrightness);
		showHelp(argBrightnessC);
		showHelp(argBacklightThreshold);
		showHelp(argBacklightColored);
		showHelp(argGamma);
		showHelp(argRAdjust);
		showHelp(argGAdjust);
		showHelp(argBAdjust);
		showHelp(argCAdjust);
		showHelp(argMAdjust);
		showHelp(argYAdjust);
		return 1;
	}
	QString hostname;
	int port {JSONAPI_DEFAULT_PORT};

	// Split hostname and port (or use default port)
	QString const givenAddress = argAddress.value(parser);

	if (!NetUtils::resolveHostPort(givenAddress, hostname, port))
	{
		emit errorManager.errorOccurred(QString("Wrong address: unable to parse address (%1)").arg(givenAddress));
		return 1;
	}

	QHostAddress hostAddress;
	if (!NetUtils::resolveHostToAddress(log, hostname, hostAddress, port))
	{
		emit errorManager.errorOccurred(QString("Address could not be resolved for hostname: \"%1\"").arg(QSTRING_CSTR(hostname)));
		return 1;
	}

	Info(log, "Connecting to Hyperion host: %s, port: %u", QSTRING_CSTR(hostAddress.toString()), port);

	// create the connection to the hyperion server
	QScopedPointer<JsonConnection> connection (new JsonConnection(hostAddress, parser.isSet(argPrint), port));

	QObject::connect(connection.get(), &JsonConnection::errorOccured, [&](const QString& error) {
		emit errorManager.errorOccurred(error);
	});

	QObject::connect(connection.get(), &JsonConnection::isReadyToSend, [&]() {
		Debug(log,"Send command to Hyperion");

		// authorization token specified. Use it first
		if (parser.isSet(argToken))
		{
			if(!connection->setToken(argToken.value(parser)))
			{
				return;
			}
		}

		//Global cammands

		if (parser.isSet(argServerInfo))
		{
			std::cout << "Server info:\n" << connection->getServerInfoString().toStdString() << "\n";
		}
		else if (parser.isSet(argSysInfo))
		{
			std::cout << "System info:\n" << connection->getSysInfo().toStdString() << "\n";
		}
		else if (parser.isSet(argSystemSuspend))
		{
			connection->suspend();
		}
		else if (parser.isSet(argSystemResume))
		{
			connection->resume();
		}
		else if (parser.isSet(argSystemToggleSuspend))
		{
			connection->toggleSuspend();
		}
		else if (parser.isSet(argSystemIdle))
		{
			connection->idle();
		}
		else if (parser.isSet(argSystemToggleIdle))
		{
			connection->toggleIdle();
		}
		else if (parser.isSet(argSystemRestart))
		{
			connection->restart();
		}
		else if (parser.isSet(argConfigGet))
		{
			QString const info = connection->getConfig("config");
			std::cout << "Configuration:\n" << info.toStdString() << "\n";
		}
		else if (parser.isSet(argSchemaGet))
		{
			QString const info = connection->getConfig("schema");
			std::cout << "Configuration Schema\n" << info.toStdString() << "\n";
		}
		else if (parser.isSet(argConfigSet))
		{
			connection->setConfig(argConfigSet.value(parser));
		}
		else
		{
			//Instance specific commands

			QJsonObject const serverInfo = connection->getServerInfo();
			if (serverInfo.isEmpty())
			{
				emit errorManager.errorOccurred("Failed to get server info");
				return;
			}

			int instanceID {-1};
			QString instanceName;

			// If a specific Hyperion instance is given, set it
			if (parser.isSet(argInstance))
			{
				instanceName = argInstance.value(parser);
			}
			// else use the first instance running

			instanceID = getRunningInstance(serverInfo,instanceName);

			if ( instanceID < 0)
			{
				return;
			}

			Debug(log, "Switching to instance-ID: %d", instanceID);
			if (!connection->setInstance(instanceID))
			{
				emit errorManager.errorOccurred(QString("Failed switching to instance %1 ").arg(instanceID));
				return;
			}

			// now execute the given instance command
			if (parser.isSet(argColor))
			{
				QList<QColor> const _cQV = argColor.getColors(parser);
				connection->setColor(_cQV, argPriority.getInt(parser), argDuration.getInt(parser));
			}
			else if (parser.isSet(argImage))
			{
				QFileInfo const imageFile {argImage.getCString(parser)};
				connection->setImage(argImage.getImage(parser), argPriority.getInt(parser), argDuration.getInt(parser), imageFile.fileName());
			}
#if defined(ENABLE_EFFECTENGINE)
			else if (parser.isSet(argEffect))
			{
				connection->setEffect(argEffect.value(parser), argEffectArgs.value(parser), argPriority.getInt(parser), argDuration.getInt(parser));
			}
			else if (parser.isSet(argCreateEffect))
			{
				connection->createEffect(argCreateEffect.value(parser), argEffectFile.value(parser), argEffectArgs.value(parser));
			}
			else if (parser.isSet(argDeleteEffect))
			{
				connection->deleteEffect(argDeleteEffect.value(parser));
			}
#endif
			else if (parser.isSet(argClear))
			{
				connection->clear(argPriority.getInt(parser));
			}
			else if (parser.isSet(argClearAll))
			{
				connection->clearAll();
			}
			else if (parser.isSet(argEnableComponent))
			{
				connection->setComponentState(argEnableComponent.value(parser), true);
			}
			else if (parser.isSet(argDisableComponent))
			{
				connection->setComponentState(argDisableComponent.value(parser), false);
			}
			else if (parser.isSet(argOn))
			{
				connection->setComponentState("ALL", true);
			}
			else if (parser.isSet(argOff))
			{
				connection->setComponentState("ALL", false);
			}
			else if (parser.isSet(argSource))
			{
				connection->setSource(argSource.getInt(parser));
			}
			else if (parser.isSet(argSourceAuto))
			{
				connection->setSourceAutoSelect();
			}
			else if (parser.isSet(argMapping))
			{
				connection->setLedMapping(argMapping.value(parser));
			}
			else if (parser.isSet(argVideoMode))
			{
				connection->setVideoMode(argVideoMode.value(parser));
			}
			else if (colorAdjust)
			{
				connection->setAdjustment(
							argId.value(parser),
							argRAdjust.getColor(parser),
							argGAdjust.getColor(parser),
							argBAdjust.getColor(parser),
							argCAdjust.getColor(parser),
							argMAdjust.getColor(parser),
							argYAdjust.getColor(parser),
							argWAdjust.getColor(parser),
							argbAdjust.getColor(parser),
							argGamma.getDoublePtr(parser),
							argGamma.getDoublePtr(parser),
							argGamma.getDoublePtr(parser),
							argBacklightThreshold.getIntPtr(parser),
							argBacklightColored.getIntPtr(parser),
							argBrightness.getIntPtr(parser),
							argBrightnessC.getIntPtr(parser)
							);
			}
		}

		connection->close();

		Debug(log, "Command execution complete. Exiting...");
		app.quit();
	});

	app.exec();

	return 0;
}
