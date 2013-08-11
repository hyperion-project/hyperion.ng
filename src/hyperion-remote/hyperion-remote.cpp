#include <QDebug>
#include <QCoreApplication>

#include <getoptPlusPlus/getoptpp.h>

using namespace vlofgren;

int main(int argc, const char * argv[])
{
    // some settings
    QString defaultServerAddress = "localhost:19444";
    int defaultPriority = 100;

    OptionsParser optionParser("Simple application to send a command to hyperion using the Json interface");
    ParameterSet & parameters = optionParser.getParameters();
    StringParameter & argAddress    = parameters.add<StringParameter>('a', "address"   , QString("Set the address of the hyperion server [default: %1]").arg(defaultServerAddress).toAscii().constData());
    IntParameter    & argPriority   = parameters.add<IntParameter>   ('p', "priority"  , QString("Use to the provided priority channel (the lower the number, the higher the priority) [default: %1]").arg(defaultPriority).toAscii().constData());
    IntParameter    & argDuration   = parameters.add<IntParameter>   ('d', "duration"  , "Specify how long the leds should be switched on in millseconds. Without this parameter, the leds will be switched on without end time.");
    StringParameter & argColor      = parameters.add<StringParameter>('c', "color"     , "Set all leds to a constant color (either RRGGBB hex value or a color name)");
    StringParameter & argImage      = parameters.add<StringParameter>('i', "image"     , "Set the leds to the colors according to the given image file");
    SwitchParameter & argList       = parameters.add<SwitchParameter>('l', "list"      , "List all priority channels which are in use");
    SwitchParameter & argClear      = parameters.add<SwitchParameter>('x', "clear"     , "Clear data for the priority channel provided by the -p option");
    SwitchParameter & argClearAll   = parameters.add<SwitchParameter>('y', "clear-all" , "Clear data for all priority channels");
    DoubleParameter & argGamma      = parameters.add<DoubleParameter>('g', "gamma"     , "Set the gamma of the leds (requires 3 values)");
    DoubleParameter & argThreshold  = parameters.add<DoubleParameter>('t', "threshold" , "Set the threshold of the leds (requires 3 values between 0.0 and 1.0)");
    DoubleParameter & argBlacklevel = parameters.add<DoubleParameter>('b', "blacklevel", "Set the blacklevel of the leds (requires 3 values which are normally between 0.0 and 1.0)");
    DoubleParameter & argWhitelevel = parameters.add<DoubleParameter>('w', "whitelevel", "Set the whitelevel of the leds (requires 3 values which are normally between 0.0 and 1.0)");
    SwitchParameter & argPrint      = parameters.add<SwitchParameter>('z', "print"     , "Print the json input and output messages on stdout");
    SwitchParameter & argHelp       = parameters.add<SwitchParameter>('h', "help"      , "Show this help message and exit");
    try
    {
        optionParser.parse(argc, argv);
    }
    catch (const std::runtime_error & e)
    {
        qWarning() << e.what();
        optionParser.usage();
        return 1;
    }

    if (argHelp.isSet())
    {
        optionParser.usage();
        return 0;
    }


    return 0;
}
