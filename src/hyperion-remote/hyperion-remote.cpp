// stl includes
#include <clocale>
#include <initializer_list>
#include <limits>
#include <iostream>
#include <stdlib.h>

// Qt includes
#include <QCoreApplication>
#include <QLocale>

// hyperion-remote include
#include "JsonConnection.h"

#include "HyperionConfig.h"
#include <commandline/Parser.h>

using namespace commandline;

/// Count the number of true values in a list of booleans
int count(std::initializer_list<bool> values)
{
	int count = 0;
	for (bool value : values) {
		if (value)
			count++;
	}
	return count;
}

void showHelp(Option & option){
	QString shortOption;
	QString longOption = QString("--%1").arg(option.names().last());

	if(option.names().size() == 2){
		shortOption = QString("-%1").arg(option.names().first());
	}

	qWarning() << qPrintable(QString("\t%1\t%2\t%3").arg(shortOption, longOption, option.description()));
}

int main(int argc, char * argv[])
{
	setenv("AVAHI_COMPAT_NOWARN", "1", 1);

	std::cout
		<< "hyperion-remote:" << std::endl
		<< "\tVersion   : " << HYPERION_VERSION << " (" << HYPERION_BUILD_ID << ")" << std::endl
		<< "\tbuild time: " << __DATE__ << " " << __TIME__ << std::endl;

	QCoreApplication app(argc, argv);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	try
	{
		// create the option parser and initialize all parameters
		Parser parser("Application to send a command to hyperion using the Json interface");

		Option          & argAddress     = parser.add<Option>       ('a', "address"   , "Set the address of the hyperion server [default: %1]", "localhost:19444");
		IntOption       & argPriority    = parser.add<IntOption>    ('p', "priority"  , "Use to the provided priority channel (suggested 2-99) [default: %1]", "50");
		IntOption       & argDuration    = parser.add<IntOption>    ('d', "duration"  , "Specify how long the leds should be switched on in milliseconds [default: infinity]");
		ColorsOption    & argColor       = parser.add<ColorsOption> ('c', "color"     , "Set all leds to a constant color (either RRGGBB hex getColors or a color name. The color may be repeated multiple time like: RRGGBBRRGGBB)");
		ImageOption     & argImage       = parser.add<ImageOption>  ('i', "image"     , "Set the leds to the colors according to the given image file");
		Option          & argEffect      = parser.add<Option>       ('e', "effect"    , "Enable the effect with the given name");
		Option          & argEffectFile  = parser.add<Option>       (0x0, "effectFile", "Arguments to use in combination with --createEffect");
		Option          & argEffectArgs  = parser.add<Option>       (0x0, "effectArgs", "Arguments to use in combination with the specified effect. Should be a Json object string.", "");
		Option          & argCreateEffect= parser.add<Option>       (0x0, "createEffect","Write a new Json Effect configuration file.\nFirst parameter = Effect name.\nSecond parameter = Effect file (--effectFile).\nLast parameter = Effect arguments (--effectArgs.)", "");
		Option          & argDeleteEffect= parser.add<Option>       (0x0, "deleteEffect","Delete a custom created Json Effect configuration file.");
		BooleanOption   & argServerInfo  = parser.add<BooleanOption>('l', "list"      , "List server info and active effects with priority and duration");
		BooleanOption   & argSysInfo     = parser.add<BooleanOption>('s', "sysinfo"   , "show system info");
		BooleanOption   & argClear       = parser.add<BooleanOption>('x', "clear"     , "Clear data for the priority channel provided by the -p option");
		BooleanOption   & argClearAll    = parser.add<BooleanOption>(0x0, "clearall"  , "Clear data for all active priority channels");
		Option          & argEnableComponent  = parser.add<Option>  ('E', "enable"    , "Enable the Component with the given name. Available Components are [SMOOTHING, BLACKBORDER, FORWARDER, UDPLISTENER, BOBLIGHT_SERVER, GRABBER, V4L, LEDDEVICE]");
		Option          & argDisableComponent = parser.add<Option>  ('D', "disable"    , "Disable the Component with the given name. Available Components are [SMOOTHING, BLACKBORDER, FORWARDER, UDPLISTENER, BOBLIGHT_SERVER, GRABBER, V4L, LEDDEVICE]");
		Option          & argId           = parser.add<Option>      ('q', "qualifier" , "Identifier(qualifier) of the adjustment to set");
		IntOption       & argBrightness   = parser.add<IntOption>   ('L', "brightness" , "Set the brightness gain of the leds");
		IntOption       & argBrightnessC  = parser.add<IntOption>   (0x0, "brightnessCompensation" , "Set the brightness compensation");
		IntOption       & argBacklightThreshold= parser.add<IntOption> ('n', "backlightThreshold" , "threshold for activating backlight (minimum brightness)");
		IntOption       & argBacklightColored  = parser.add<IntOption>    (0x0, "backlightColored" , "0 = white backlight; 1 =  colored backlight");
		DoubleOption    & argGamma       = parser.add<DoubleOption>  ('g', "gamma"     , "Set the overall gamma of the leds");
		BooleanOption   & argPrint       = parser.add<BooleanOption>(0x0, "print"     , "Print the json input and output messages on stdout");
		BooleanOption   & argHelp        = parser.add<BooleanOption>('h', "help"      , "Show this help message and exit");
		ColorOption     & argRAdjust     = parser.add<ColorOption>  ('R', "redAdjustment" , "Set the adjustment of the red color (requires colors in hex format as RRGGBB)");
		ColorOption     & argGAdjust     = parser.add<ColorOption>  ('G', "greenAdjustment", "Set the adjustment of the green color (requires colors in hex format as RRGGBB)");
		ColorOption     & argBAdjust     = parser.add<ColorOption>  ('B', "blueAdjustment", "Set the adjustment of the blue color (requires colors in hex format as RRGGBB)");
		ColorOption     & argCAdjust     = parser.add<ColorOption>  ('C', "cyanAdjustment" , "Set the adjustment of the cyan color (requires colors in hex format as RRGGBB)");
		ColorOption     & argMAdjust     = parser.add<ColorOption>  ('M', "magentaAdjustment", "Set the adjustment of the magenta color (requires colors in hex format as RRGGBB)");
		ColorOption     & argYAdjust     = parser.add<ColorOption>  ('Y', "yellowAdjustment", "Set the adjustment of the yellow color (requires colors in hex format as RRGGBB)");
		ColorOption     & argWAdjust     = parser.add<ColorOption>  ('W', "whiteAdjustment", "Set the adjustment of the white color (requires colors in hex format as RRGGBB)");
		ColorOption     & argbAdjust     = parser.add<ColorOption>  ('b', "blackAdjustment", "Set the adjustment of the black color (requires colors in hex format as RRGGBB)");
		Option          & argMapping     = parser.add<Option>       ('m', "ledMapping"   , "Set the methode for image to led mapping valid values: multicolor_mean, unicolor_mean");
		Option          & argVideoMode   = parser.add<Option>       ('V', "videoMode"   , "Set the video mode valid values: 2D, 3DSBS, 3DTAB");
		IntOption       & argSource      = parser.add<IntOption>    (0x0, "sourceSelect"  , "Set current active priority channel and deactivate auto source switching");
		BooleanOption   & argSourceAuto  = parser.add<BooleanOption>(0x0, "sourceAutoSelect", "Enables auto source, if disabled prio by manual selecting input source");
		BooleanOption   & argOff         = parser.add<BooleanOption>(0x0, "off", "deactivates hyperion");
		BooleanOption   & argOn          = parser.add<BooleanOption>(0x0, "on", "activates hyperion");
		BooleanOption   & argConfigGet   = parser.add<BooleanOption>(0x0, "configGet"  , "Print the current loaded Hyperion configuration file");
		BooleanOption   & argSchemaGet   = parser.add<BooleanOption>(0x0, "schemaGet"  , "Print the json schema for Hyperion configuration");
		Option          & argConfigSet   = parser.add<Option>       (0x0, "configSet", "Write to the actual loaded configuration file. Should be a Json object string.");

		// parse all _options
		parser.process(app);

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
		int commandCount = count({ parser.isSet(argColor), parser.isSet(argImage), parser.isSet(argEffect), parser.isSet(argCreateEffect), parser.isSet(argDeleteEffect),
		    parser.isSet(argServerInfo), parser.isSet(argSysInfo),parser.isSet(argClear), parser.isSet(argClearAll), parser.isSet(argEnableComponent), parser.isSet(argDisableComponent), colorAdjust,
		    parser.isSet(argSource), parser.isSet(argSourceAuto), parser.isSet(argOff), parser.isSet(argOn), parser.isSet(argConfigGet), parser.isSet(argSchemaGet), parser.isSet(argConfigSet),
		    parser.isSet(argMapping),parser.isSet(argVideoMode) });
		if (commandCount != 1)
		{
			qWarning() << (commandCount == 0 ? "No command found." : "Multiple commands found.") << " Provide exactly one of the following options:";
			showHelp(argColor);
			showHelp(argImage);
			showHelp(argEffect);
			showHelp(argCreateEffect);
			showHelp(argDeleteEffect);
			showHelp(argServerInfo);
			showHelp(argSysInfo);
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

		// create the connection to the hyperion server
		JsonConnection connection(argAddress.value(parser), parser.isSet(argPrint));

		// now execute the given command
		if (parser.isSet(argColor))
		{
			// TODO: make sure setColor accepts a QList<QColor>
			connection.setColor(argColor.getColors(parser).toVector().toStdVector(), argPriority.getInt(parser), argDuration.getInt(parser));
		}
		else if (parser.isSet(argImage))
		{
			connection.setImage(argImage.getImage(parser), argPriority.getInt(parser), argDuration.getInt(parser));
		}
		else if (parser.isSet(argEffect))
		{
			connection.setEffect(argEffect.value(parser), argEffectArgs.value(parser), argPriority.getInt(parser), argDuration.getInt(parser));
		}
		else if (parser.isSet(argCreateEffect))
		{
			connection.createEffect(argCreateEffect.value(parser), argEffectFile.value(parser), argEffectArgs.value(parser));
		}
		else if (parser.isSet(argDeleteEffect))
		{
			connection.deleteEffect(argDeleteEffect.value(parser));
		}
		else if (parser.isSet(argServerInfo))
		{
			std::cout << "Server info:\n" << connection.getServerInfo().toStdString() << std::endl;
		}
		else if (parser.isSet(argSysInfo))
		{
			std::cout << "System info:\n" << connection.getSysInfo().toStdString() << std::endl;
		}
		else if (parser.isSet(argClear))
		{
			connection.clear(argPriority.getInt(parser));
		}
		else if (parser.isSet(argClearAll))
		{
			connection.clearAll();
		}
		else if (parser.isSet(argEnableComponent))
		{
			connection.setComponentState(argEnableComponent.value(parser), true);
		}
		else if (parser.isSet(argDisableComponent))
		{
			connection.setComponentState(argDisableComponent.value(parser), false);
		}
		else if (parser.isSet(argOn))
		{
			connection.setComponentState("ALL", true);
		}
		else if (parser.isSet(argOff))
		{
			connection.setComponentState("ALL", false);
		}
		else if (parser.isSet(argSource))
		{
			connection.setSource(argSource.getInt(parser));
		}
		else if (parser.isSet(argSourceAuto))
		{
			connection.setSourceAutoSelect();
		}
		else if (parser.isSet(argConfigGet))
		{
			QString info = connection.getConfig("config");
			std::cout << "Configuration:\n" << info.toStdString() << std::endl;
		}
		else if (parser.isSet(argSchemaGet))
		{
			QString info = connection.getConfig("schema");
			std::cout << "Configuration Schema\n" << info.toStdString() << std::endl;
		}
		else if (parser.isSet(argConfigSet))
		{
			connection.setConfig(argConfigSet.value(parser));
		}
		else if (parser.isSet(argMapping))
		{
			connection.setLedMapping(argMapping.value(parser));
		}
		else if (parser.isSet(argVideoMode))
		{
			connection.setVideoMode(argVideoMode.value(parser));
		}
		else if (colorAdjust)
		{
			connection.setAdjustment(
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
	catch (const std::runtime_error & e)
	{
		// An error occured. Display error and quit
		std::cerr << e.what() << std::endl;
		return 1;
	}

	return 0;
}
