
// C++ includes
#include <csignal>

// QT includes
#include <QCoreApplication>

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <bootsequence/RainbowBootSequence.h>

// Dispmanx grabber includes
#include <dispmanx-grabber/DispmanxWrapper.h>

// XBMC Video checker includes
#include <xbmcvideochecker/XBMCVideoChecker.h>

// JsonServer includes
#include <jsonserver/JsonServer.h>

void signal_handler(const int signum)
{
	QCoreApplication::quit();
}

int main(int argc, char** argv)
{
	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);
	std::cout << "QCoreApplication initialised" << std::endl;

	signal(SIGINT,  signal_handler);
	signal(SIGTERM, signal_handler);

	if (argc < 2)
	{
		std::cout << "Missing required configuration file. Usage:" << std::endl;
		std::cout << "hyperiond [config.file]" << std::endl;
		return 0;
	}

	const std::string configFile = argv[1];
	std::cout << "Selected configuration file: " << configFile.c_str() << std::endl;

	Hyperion hyperion(configFile);
	std::cout << "Hyperion created and initialised" << std::endl;

	RainbowBootSequence bootSequence(&hyperion);
	bootSequence.start();

	XBMCVideoChecker xbmcVideoChecker("127.0.0.1", 1000, &hyperion, 127);
	xbmcVideoChecker.start();

	DispmanxWrapper dispmanx(64, 64, 10, &hyperion);
	dispmanx.start();
	std::cout << "Frame grabber created and started" << std::endl;

	JsonServer jsonServer(&hyperion);
	std::cout << "Json server created and started on port " << jsonServer.getPort() << std::endl;

	app.exec();
	std::cout << "Application closed" << std::endl;

	// Stop the frame grabber
	dispmanx.stop();
	// Clear all colors (switchting off all leds)
	hyperion.clearall();
}
