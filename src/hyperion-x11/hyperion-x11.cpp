
// QT includes
#include <QCoreApplication>
#include <QImage>

// getoptPlusPLus includes
#include <getoptPlusPlus/getoptpp.h>

#include "protoserver/ProtoConnectionWrapper.h"
#include "X11Wrapper.h"

using namespace vlofgren;

// save the image as screenshot
void saveScreenshot(const char * filename, const Image<ColorRgb> & image)
{
    // store as PNG
    QImage pngImage((const uint8_t *) image.memptr(), image.width(), image.height(), 3*image.width(), QImage::Format_RGB888);
    pngImage.save(filename);
}

int main(int argc, char ** argv)
{
    QCoreApplication app(argc, argv);

    try
    {
        // create the option parser and initialize all parameters
        OptionsParser optionParser("X11 capture application for Hyperion");
        ParameterSet & parameters = optionParser.getParameters();

        IntParameter           & argFps             = parameters.add<IntParameter>          ('f', "framerate",        "Capture frame rate [default=10]");
        IntParameter           & argCropWidth       = parameters.add<IntParameter>          (0x0, "crop-width",       "Number of pixels to crop from the left and right sides of the picture before decimation [default=0]");
        IntParameter           & argCropHeight      = parameters.add<IntParameter>          (0x0, "crop-height",      "Number of pixels to crop from the top and the bottom of the picture before decimation [default=0]");
        IntParameter           & argCropLeft        = parameters.add<IntParameter>          (0x0, "crop-left",        "Number of pixels to crop from the left of the picture before decimation (overrides --crop-width)");
        IntParameter           & argCropRight       = parameters.add<IntParameter>          (0x0, "crop-right",       "Number of pixels to crop from the right of the picture before decimation (overrides --crop-width)");
        IntParameter           & argCropTop         = parameters.add<IntParameter>          (0x0, "crop-top",         "Number of pixels to crop from the top of the picture before decimation (overrides --crop-height)");
        IntParameter           & argCropBottom      = parameters.add<IntParameter>          (0x0, "crop-bottom",      "Number of pixels to crop from the bottom of the picture before decimation (overrides --crop-height)");
        IntParameter           & argSizeDecimation  = parameters.add<IntParameter>          ('s', "size-decimator",   "Decimation factor for the output size [default=8]");
        SwitchParameter<>      & argScreenshot      = parameters.add<SwitchParameter<>>     (0x0, "screenshot",       "Take a single screenshot, save it to file and quit");
        StringParameter        & argAddress         = parameters.add<StringParameter>       ('a', "address",          "Set the address of the hyperion server [default: 127.0.0.1:19445]");
        IntParameter           & argPriority        = parameters.add<IntParameter>          ('p', "priority",         "Use the provided priority channel (the lower the number, the higher the priority) [default: 800]");
        SwitchParameter<>      & argSkipReply       = parameters.add<SwitchParameter<>>     (0x0, "skip-reply",       "Do not receive and check reply messages from Hyperion");
        SwitchParameter<>      & argHelp            = parameters.add<SwitchParameter<>>     ('h', "help",             "Show this help message and exit");

        // set defaults
        argFps.setDefault(10);
        argCropWidth.setDefault(0);
        argCropHeight.setDefault(0);
        argSizeDecimation.setDefault(8);
        argAddress.setDefault("127.0.0.1:19445");
        argPriority.setDefault(800);

        // parse all options
        optionParser.parse(argc, const_cast<const char **>(argv));

        // check if we need to display the usage. exit if we do.
        if (argHelp.isSet())
        {
            optionParser.usage();
            return 0;
        }

        // cropping values if not defined
        if (!argCropLeft.isSet())   argCropLeft.setDefault(argCropWidth.getValue());
        if (!argCropRight.isSet())  argCropRight.setDefault(argCropWidth.getValue());
        if (!argCropTop.isSet())    argCropTop.setDefault(argCropHeight.getValue());
        if (!argCropBottom.isSet()) argCropBottom.setDefault(argCropHeight.getValue());

        // Create the X11 grabbing stuff
        int grabInterval = 1000 / argFps.getValue();
        X11Wrapper x11Wrapper(
                    grabInterval,
                    argCropLeft.getValue(),
                    argCropRight.getValue(),
                    argCropTop.getValue(),
                    argCropBottom.getValue(),
                    argSizeDecimation.getValue(), // horizontal decimation
                    argSizeDecimation.getValue()); // vertical decimation

        if (argScreenshot.isSet())
        {
            // Capture a single screenshot and finish
            const Image<ColorRgb> & screenshot = x11Wrapper.getScreenshot();
            saveScreenshot("screenshot.png", screenshot);
        }
        else
        {
            // Create the Proto-connection with hyperiond
            ProtoConnectionWrapper protoWrapper(argAddress.getValue(), argPriority.getValue(), 1000, argSkipReply.isSet());

            // Connect the screen capturing to the proto processing
            QObject::connect(&x11Wrapper, SIGNAL(sig_screenshot(const Image<ColorRgb> &)), &protoWrapper, SLOT(receiveImage(Image<ColorRgb>)));

            // Start the capturing
            x11Wrapper.start();

            // Start the application
            app.exec();
        }
    }
    catch (const std::runtime_error & e)
    {
        // An error occured. Display error and quit
        std::cerr << e.what() << std::endl;
        return -1;
    }

    return 0;
}
