#include <initializer_list>

#include <QCoreApplication>

#include <getoptPlusPlus/getoptpp.h>

#include "specialoptions.h"
#include "connection.h"


using namespace vlofgren;

int count(std::initializer_list<bool> values)
{
    int count = 0;
    for (auto& value : values) {
        if (value)
            count++;
    }
    return count;
}

int main(int argc, char * argv[])
{
    QCoreApplication app(argc, argv);

    try
    {
        // some settings
        QString defaultServerAddress = "localhost:19444";
        int defaultPriority = 100;

        OptionsParser optionParser("Simple application to send a command to hyperion using the Json interface");
        ParameterSet & parameters = optionParser.getParameters();
        StringParameter    & argAddress    = parameters.add<StringParameter>   ('a', "address"   , QString("Set the address of the hyperion server [default: %1]").arg(defaultServerAddress).toAscii().constData());
        IntParameter       & argPriority   = parameters.add<IntParameter>      ('p', "priority"  , QString("Use to the provided priority channel (the lower the number, the higher the priority) [default: %1]").arg(defaultPriority).toAscii().constData());
        IntParameter       & argDuration   = parameters.add<IntParameter>      ('d', "duration"  , "Specify how long the leds should be switched on in millseconds [default: infinity]");
        ColorParameter     & argColor      = parameters.add<ColorParameter>    ('c', "color"     , "Set all leds to a constant color (either RRGGBB hex value or a color name)");
        ImageParameter     & argImage      = parameters.add<ImageParameter>    ('i', "image"     , "Set the leds to the colors according to the given image file");
        SwitchParameter<>  & argList       = parameters.add<SwitchParameter<> >('l', "list"      , "List all priority channels which are in use");
        SwitchParameter<>  & argClear      = parameters.add<SwitchParameter<> >('x', "clear"     , "Clear data for the priority channel provided by the -p option");
        SwitchParameter<>  & argClearAll   = parameters.add<SwitchParameter<> >(0x0, "clear-all" , "Clear data for all priority channels");
        TransformParameter & argGamma      = parameters.add<TransformParameter>('g', "gamma"     , "Set the gamma of the leds (requires 3 values)");
        TransformParameter & argThreshold  = parameters.add<TransformParameter>('t', "threshold" , "Set the threshold of the leds (requires 3 space seperated values between 0.0 and 1.0)");
        TransformParameter & argBlacklevel = parameters.add<TransformParameter>('b', "blacklevel", "Set the blacklevel of the leds (requires 3 space seperated values which are normally between 0.0 and 1.0)");
        TransformParameter & argWhitelevel = parameters.add<TransformParameter>('w', "whitelevel", "Set the whitelevel of the leds (requires 3 space seperated values which are normally between 0.0 and 1.0)");
        SwitchParameter<>  & argPrint      = parameters.add<SwitchParameter<> >(0x0, "print"     , "Print the json input and output messages on stdout");
        SwitchParameter<>  & argHelp       = parameters.add<SwitchParameter<> >('h', "help"      , "Show this help message and exit");

        argAddress.setDefault(defaultServerAddress.toStdString());
        argPriority.setDefault(defaultPriority);
        argDuration.setDefault(-1);

        // parse the options
        optionParser.parse(argc, const_cast<const char **>(argv));

        // check if we need to display the usage
        if (argHelp.isSet())
        {
            optionParser.usage();
            return 0;
        }

        // check if a color transform is set
        bool colorTransform = argThreshold.isSet() || argGamma.isSet() || argBlacklevel.isSet() || argWhitelevel.isSet();

        // check if exactly one command was given
        int commandCount = count({argColor.isSet(), argImage.isSet(), argList.isSet(), argClear.isSet(), argClearAll.isSet(), colorTransform});
        if (commandCount != 1)
        {
            if (commandCount == 0)
            {
                std::cerr << "No command found. Provide one of the following options:" << std::endl;
            }
            else
            {
                std::cerr << "Multiple commands found. Provide one of the following options:" << std::endl;
            }
            std::cerr << "  " << argColor.usageLine() << std::endl;
            std::cerr << "  " << argImage.usageLine() << std::endl;
            std::cerr << "  " << argList.usageLine() << std::endl;
            std::cerr << "  " << argClear.usageLine() << std::endl;
            std::cerr << "  " << argClearAll.usageLine() << std::endl;
            std::cerr << "or one or more of the color transformations:" << std::endl;
            std::cerr << "  " << argThreshold.usageLine() << std::endl;
            std::cerr << "  " << argGamma.usageLine() << std::endl;
            std::cerr << "  " << argBlacklevel.usageLine() << std::endl;
            std::cerr << "  " << argWhitelevel.usageLine() << std::endl;
            return 1;
        }

        // create the connection
        Connection connection(argAddress.getValue(), argPrint.isSet());

        // now execute the given command
        if (argColor.isSet())
        {
            connection.setColor(argColor.getValue(), argPriority.getValue(), argDuration.getValue());
        }
        else if (argImage.isSet())
        {
            connection.setImage(argImage.getValue(), argPriority.getValue(), argDuration.getValue());
        }
        else if (argList.isSet())
        {
            connection.listPriorities();
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
            ColorTransform threshold  = argThreshold.getValue();
            ColorTransform gamma      = argGamma.getValue();
            ColorTransform blacklevel = argBlacklevel.getValue();
            ColorTransform whitelevel = argWhitelevel.getValue();

            connection.setTransform(
                        argThreshold.isSet()  ? &threshold  : nullptr,
                        argGamma.isSet()      ? &gamma      : nullptr,
                        argBlacklevel.isSet() ? &blacklevel : nullptr,
                        argWhitelevel.isSet() ? &whitelevel : nullptr);
        }
    }
    catch (const std::runtime_error & e)
    {
        std::cerr << e.what() << std::endl;
        return 1;
    }

    return 0;
}
