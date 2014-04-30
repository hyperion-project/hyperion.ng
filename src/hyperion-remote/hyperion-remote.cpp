// stl includes
#include <clocale>
#include <initializer_list>

// Qt includes
#include <QCoreApplication>
#include <QLocale>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

// hyperion-remote include
#include "CustomParameter.h"
#include "JsonConnection.h"

using namespace vlofgren;

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

int main(int argc, char * argv[])
{
	QCoreApplication app(argc, argv);

	// force the locale
	setlocale(LC_ALL, "C");
	QLocale::setDefault(QLocale::c());

	try
	{
		// some default settings
		QString defaultServerAddress = "localhost:19444";
		int defaultPriority = 100;

		// create the option parser and initialize all parameters
		OptionsParser optionParser("Simple application to send a command to hyperion using the Json interface");
		ParameterSet & parameters = optionParser.getParameters();
		StringParameter    & argAddress    = parameters.add<StringParameter>   ('a', "address"   , QString("Set the address of the hyperion server [default: %1]").arg(defaultServerAddress).toAscii().constData());
		IntParameter       & argPriority   = parameters.add<IntParameter>      ('p', "priority"  , QString("Use to the provided priority channel (the lower the number, the higher the priority) [default: %1]").arg(defaultPriority).toAscii().constData());
		IntParameter       & argDuration   = parameters.add<IntParameter>      ('d', "duration"  , "Specify how long the leds should be switched on in millseconds [default: infinity]");
		ColorParameter     & argColor      = parameters.add<ColorParameter>    ('c', "color"     , "Set all leds to a constant color (either RRGGBB hex value or a color name. The color may be repeated multiple time like: RRGGBBRRGGBB)");
		ImageParameter     & argImage      = parameters.add<ImageParameter>    ('i', "image"     , "Set the leds to the colors according to the given image file");
        StringParameter    & argEffect     = parameters.add<StringParameter>   ('e', "effect"    , "Enable the effect with the given name");
        StringParameter    & argEffectArgs = parameters.add<StringParameter>   (0x0, "effectArgs", "Arguments to use in combination with the specified effect. Should be a Json object string.");
		SwitchParameter<>  & argServerInfo = parameters.add<SwitchParameter<> >('l', "list"      , "List server info");
		SwitchParameter<>  & argClear      = parameters.add<SwitchParameter<> >('x', "clear"     , "Clear data for the priority channel provided by the -p option");
		SwitchParameter<>  & argClearAll   = parameters.add<SwitchParameter<> >(0x0, "clearall"  , "Clear data for all active priority channels");
		StringParameter    & argId         = parameters.add<StringParameter>   ('q', "qualifier" , "Identifier(qualifier) of the transform to set");
		DoubleParameter    & argSaturation = parameters.add<DoubleParameter>   ('s', "saturation", "Set the HSV saturation gain of the leds");
		DoubleParameter    & argValue      = parameters.add<DoubleParameter>   ('v', "value"     , "Set the HSV value gain of the leds");
		TransformParameter & argGamma      = parameters.add<TransformParameter>('g', "gamma"     , "Set the gamma of the leds (requires 3 space seperated values)");
		TransformParameter & argThreshold  = parameters.add<TransformParameter>('t', "threshold" , "Set the threshold of the leds (requires 3 space seperated values between 0.0 and 1.0)");
		TransformParameter & argBlacklevel = parameters.add<TransformParameter>('b', "blacklevel", "Set the blacklevel of the leds (requires 3 space seperated values which are normally between 0.0 and 1.0)");
		TransformParameter & argWhitelevel = parameters.add<TransformParameter>('w', "whitelevel", "Set the whitelevel of the leds (requires 3 space seperated values which are normally between 0.0 and 1.0)");
		SwitchParameter<>  & argPrint      = parameters.add<SwitchParameter<> >(0x0, "print"     , "Print the json input and output messages on stdout");
		SwitchParameter<>  & argHelp       = parameters.add<SwitchParameter<> >('h', "help"      , "Show this help message and exit");

		// set the default values
		argAddress.setDefault(defaultServerAddress.toStdString());
		argPriority.setDefault(defaultPriority);
		argDuration.setDefault(-1);
		argEffectArgs.setDefault("");

		// parse all options
		optionParser.parse(argc, const_cast<const char **>(argv));

		// check if we need to display the usage. exit if we do.
		if (argHelp.isSet())
		{
			optionParser.usage();
			return 0;
		}

		// check if at least one of the available color transforms is set
		bool colorTransform = argSaturation.isSet() || argValue.isSet() || argThreshold.isSet() || argGamma.isSet() || argBlacklevel.isSet() || argWhitelevel.isSet();

		// check that exactly one command was given
        int commandCount = count({argColor.isSet(), argImage.isSet(), argEffect.isSet(), argServerInfo.isSet(), argClear.isSet(), argClearAll.isSet(), colorTransform});
		if (commandCount != 1)
		{
			std::cerr << (commandCount == 0 ? "No command found." : "Multiple commands found.") << " Provide exactly one of the following options:" << std::endl;
			std::cerr << "  " << argColor.usageLine() << std::endl;
			std::cerr << "  " << argImage.usageLine() << std::endl;
            std::cerr << "  " << argEffect.usageLine() << std::endl;
			std::cerr << "  " << argServerInfo.usageLine() << std::endl;
			std::cerr << "  " << argClear.usageLine() << std::endl;
			std::cerr << "  " << argClearAll.usageLine() << std::endl;
			std::cerr << "or one or more of the available color transformations:" << std::endl;
			std::cerr << "  " << argId.usageLine() << std::endl;
			std::cerr << "  " << argSaturation.usageLine() << std::endl;
			std::cerr << "  " << argValue.usageLine() << std::endl;
			std::cerr << "  " << argThreshold.usageLine() << std::endl;
			std::cerr << "  " << argGamma.usageLine() << std::endl;
			std::cerr << "  " << argBlacklevel.usageLine() << std::endl;
			std::cerr << "  " << argWhitelevel.usageLine() << std::endl;
			return 1;
		}

		// create the connection to the hyperion server
		JsonConnection connection(argAddress.getValue(), argPrint.isSet());

		// now execute the given command
		if (argColor.isSet())
		{
			connection.setColor(argColor.getValue(), argPriority.getValue(), argDuration.getValue());
		}
		else if (argImage.isSet())
		{
			connection.setImage(argImage.getValue(), argPriority.getValue(), argDuration.getValue());
		}
        else if (argEffect.isSet())
        {
            connection.setEffect(argEffect.getValue(), argEffectArgs.getValue(), argPriority.getValue(), argDuration.getValue());
        }
        else if (argServerInfo.isSet())
		{
			QString info = connection.getServerInfo();
			std::cout << "Server info:\n" << info.toStdString() << std::endl;
		}
		else if (argClear.isSet())
		{
			connection.clear(argPriority.getValue());
		}
		else if (argClearAll.isSet())
		{
			connection.clearAll();
		}
		else if (colorTransform)
		{
			std::string transId;
			double saturation, value;
			ColorTransformValues threshold, gamma, blacklevel, whitelevel;

			if (argId.isSet())         transId    = argId.getValue();
			if (argSaturation.isSet()) saturation = argSaturation.getValue();
			if (argValue.isSet())      value      = argValue.getValue();
			if (argThreshold.isSet())  threshold  = argThreshold.getValue();
			if (argGamma.isSet())      gamma      = argGamma.getValue();
			if (argBlacklevel.isSet()) blacklevel = argBlacklevel.getValue();
			if (argWhitelevel.isSet()) whitelevel = argWhitelevel.getValue();

			connection.setTransform(
						argId.isSet()         ? &transId    : nullptr,
						argSaturation.isSet() ? &saturation : nullptr,
						argValue.isSet()      ? &value      : nullptr,
						argThreshold.isSet()  ? &threshold  : nullptr,
						argGamma.isSet()      ? &gamma      : nullptr,
						argBlacklevel.isSet() ? &blacklevel : nullptr,
						argWhitelevel.isSet() ? &whitelevel : nullptr);
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
