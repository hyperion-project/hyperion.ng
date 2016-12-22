// stl includes
#include <clocale>
#include <initializer_list>
#include <limits>
#include <iostream>

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
		Parser parser("Simple application to send a command to hyperion using the Json interface");

		Option          & argAddress     = parser.add<Option>       ('a', "address"   , "Set the address of the hyperion server [default: %1]", "localhost:19444");
		IntOption       & argPriority    = parser.add<IntOption>    ('p', "priority"  , "Use to the provided priority channel (the lower the number, the higher the priority) [default: %1]", "100");
		IntOption       & argDuration    = parser.add<IntOption>    ('d', "duration"  , "Specify how long the leds should be switched on in milliseconds [default: infinity]");
		ColorsOption    & argColor       = parser.add<ColorsOption> ('c', "color"     , "Set all leds to a constant color (either RRGGBB hex getColors or a color name. The color may be repeated multiple time like: RRGGBBRRGGBB)");
		ImageOption     & argImage       = parser.add<ImageOption>  ('i', "image"     , "Set the leds to the colors according to the given image file");
		Option          & argEffect      = parser.add<Option>       ('e', "effect"    , "Enable the effect with the given name");
		Option          & argEffectFile  = parser.add<Option>       (0x0, "effectFile", "Arguments to use in combination with --createEffect");
		Option          & argEffectArgs  = parser.add<Option>       (0x0, "effectArgs", "Arguments to use in combination with the specified effect. Should be a Json object string.", "");
		Option          & argCreateEffect= parser.add<Option>       (0x0, "createEffect","Write a new Json Effect configuration file.\nFirst parameter = Effect name.\nSecond parameter = Effect file (--effectFile).\nLast parameter = Effect arguments (--effectArgs.)", "");
		Option          & argDeleteEffect= parser.add<Option>       (0x0, "deleteEffect","Delete a custom created Json Effect configuration file.");
		BooleanOption   & argServerInfo  = parser.add<BooleanOption>('l', "list"      , "List server info and active effects with priority and duration");
		BooleanOption   & argClear       = parser.add<BooleanOption>('x', "clear"     , "Clear data for the priority channel provided by the -p option");
		BooleanOption   & argClearAll    = parser.add<BooleanOption>(0x0, "clearall"  , "Clear data for all active priority channels");
		Option          & argEnableComponent  = parser.add<Option>  ('E', "enable"    , "Enable the Component with the given name. Available Components are [SMOOTHING, BLACKBORDER, KODICHECKER, FORWARDER, UDPLISTENER, BOBLIGHT_SERVER, GRABBER, V4L]");
		Option          & argDisableComponent = parser.add<Option>  ('D', "disable"    , "Disable the Component with the given name. Available Components are [SMOOTHING, BLACKBORDER, KODICHECKER, FORWARDER, UDPLISTENER, BOBLIGHT_SERVER, GRABBER, V4L]");
		Option          & argId          = parser.add<Option>       ('q', "qualifier" , "Identifier(qualifier) of the transform to set");
		DoubleOption    & argSaturation  = parser.add<DoubleOption> ('s', "saturation", "!DEPRECATED! Will be removed soon! Set the HSV saturation gain of the leds");
		DoubleOption    & argValue       = parser.add<DoubleOption> ('v', "getColors"     , "!DEPRECATED! Will be removed soon! Set the HSV getColors gain of the leds");
		DoubleOption    & argSaturationL = parser.add<DoubleOption> ('u', "saturationL", "Set the HSL saturation gain of the leds");
		DoubleOption    & argLuminance   = parser.add<DoubleOption> ('m', "luminance" , "Set the HSL luminance gain of the leds");
		DoubleOption    & argLuminanceMin= parser.add<DoubleOption> ('n', "luminanceMin" , "Set the HSL luminance minimum of the leds (backlight)");
		ColorOption     & argGamma       = parser.add<ColorOption>  ('g', "gamma"     , "Set the gamma of the leds (requires colors in hex format as RRGGBB)");
		ColorOption     & argThreshold   = parser.add<ColorOption>  ('t', "threshold" , "Set the threshold of the leds (requires colors in hex format as RRGGBB)");
		ColorOption     & argBlacklevel  = parser.add<ColorOption>  ('b', "blacklevel", "!DEPRECATED! Will be removed soon! Set the blacklevel of the leds (requires colors in hex format as RRGGBB which are normally between 0.0 and 1.0)");
		ColorOption     & argWhitelevel  = parser.add<ColorOption>  ('w', "whitelevel", "!DEPRECATED! Will be removed soon! Set the whitelevel of the leds (requires colors in hex format as RRGGBB which are normally between 0.0 and 1.0)");
		BooleanOption   & argPrint       = parser.add<BooleanOption>(0x0, "print"     , "Print the json input and output messages on stdout");
		BooleanOption   & argHelp        = parser.add<BooleanOption>('h', "help"      , "Show this help message and exit");
		Option          & argIdA         = parser.add<Option>       ('j', "qualifier-a" , "Identifier(qualifier) of the adjustment to set");
		ColorOption     & argRAdjust     = parser.add<ColorOption>  ('R', "redAdjustment" , "Set the adjustment of the red color (requires colors in hex format as RRGGBB)");
		ColorOption     & argGAdjust     = parser.add<ColorOption>  ('G', "greenAdjustment", "Set the adjustment of the green color (requires colors in hex format as RRGGBB)");
		ColorOption     & argBAdjust     = parser.add<ColorOption>  ('B', "blueAdjustment", "Set the adjustment of the blue color (requires colors in hex format as RRGGBB)");
		Option          & argMapping     = parser.add<Option>       ('M', "ledMapping"   , "Set the methode for image to led mapping valif values: multicolor:mean, unicolor_mean");
		IntOption       & argSource      = parser.add<IntOption>    (0x0, "sourceSelect"  , "Set current active priority channel and deactivate auto source switching");
		BooleanOption   & argSourceAuto  = parser.add<BooleanOption>(0x0, "sourceAutoSelect", "Enables auto source, if disabled prio by manual selecting input source");
		BooleanOption   & argSourceOff   = parser.add<BooleanOption>(0x0, "sourceOff", "select no source, this results in leds activly set to black (=off)");
		BooleanOption   & argConfigGet   = parser.add<BooleanOption>(0x0, "configGet"  , "Print the current loaded Hyperion configuration file");
		BooleanOption   & argSchemaGet   = parser.add<BooleanOption>(0x0, "schemaGet"  , "Print the json schema for Hyperion configuration");
		Option          & argConfigSet   = parser.add<Option>       ('W', "configSet", "Write to the actual loaded configuration file. Should be a Json object string.");

		// parse all _options
		parser.process(app);

		// check if we need to display the usage. exit if we do.
		if (parser.isSet(argHelp))
		{
			parser.showHelp(0);
		}

		// check if at least one of the available color transforms is set
		bool colorTransform = parser.isSet(argSaturation) || parser.isSet(argValue) || parser.isSet(argSaturationL) || parser.isSet(argLuminance)
		                   || parser.isSet(argLuminanceMin) || parser.isSet(argThreshold) || parser.isSet(argGamma) || parser.isSet(argBlacklevel) || parser.isSet(argWhitelevel);
		bool colorAdjust = parser.isSet(argRAdjust) || parser.isSet(argGAdjust) || parser.isSet(argBAdjust);
		bool colorModding = colorTransform || colorAdjust;
		
		// check that exactly one command was given
		int commandCount = count({ parser.isSet(argColor), parser.isSet(argImage), parser.isSet(argEffect), parser.isSet(argCreateEffect), parser.isSet(argDeleteEffect), 
		    parser.isSet(argServerInfo), parser.isSet(argClear), parser.isSet(argClearAll), parser.isSet(argEnableComponent), parser.isSet(argDisableComponent), colorModding,
		    parser.isSet(argSource), parser.isSet(argSourceAuto), parser.isSet(argSourceOff), parser.isSet(argConfigGet), parser.isSet(argSchemaGet), parser.isSet(argConfigSet),
			parser.isSet(argMapping) });
		if (commandCount != 1)
		{
			qWarning() << (commandCount == 0 ? "No command found." : "Multiple commands found.") << " Provide exactly one of the following options:";
			showHelp(argColor);
			showHelp(argImage);
			showHelp(argEffect);
			showHelp(argCreateEffect);
			showHelp(argDeleteEffect);
			showHelp(argServerInfo);
			showHelp(argClear);
			showHelp(argClearAll);
			showHelp(argEnableComponent);
			showHelp(argDisableComponent);
			showHelp(argSource);
			showHelp(argSourceAuto);
			showHelp(argConfigGet);
			qWarning() << "or one or more of the available color modding operations:";
			showHelp(argId);
			showHelp(argSaturation);
			showHelp(argValue);
			showHelp(argSaturationL);
			showHelp(argLuminance);
			showHelp(argLuminanceMin);
			showHelp(argThreshold);
			showHelp(argGamma);
			showHelp(argBlacklevel);
			showHelp(argWhitelevel);
			showHelp(argIdA);
			showHelp(argRAdjust);
			showHelp(argGAdjust);
			showHelp(argBAdjust);
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
			QString info = connection.getServerInfo();
			std::cout << "Server info:\n" << info.toStdString() << std::endl;
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
		else if (parser.isSet(argSourceOff))
		{
			connection.setSource(std::numeric_limits<int>::max());
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
		else if (colorModding)
		{	
			if (colorAdjust)
			{
				connection.setAdjustment(
                    argIdA.value(parser),
					argRAdjust.getColor(parser),
					argGAdjust.getColor(parser),
					argBAdjust.getColor(parser)
				);

			}
			if (colorTransform)
			{
				connection.setTransform(
                    argId.value(parser),
					argSaturation.getDoublePtr(parser),
					argValue.getDoublePtr(parser),
                    argSaturationL.getDoublePtr(parser),
					argLuminance.getDoublePtr(parser),
					argLuminanceMin.getDoublePtr(parser),
					argThreshold.getColor(parser),
					argGamma.getColor(parser),
					argBlacklevel.getColor(parser),
					argWhitelevel.getColor(parser));
			}
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
