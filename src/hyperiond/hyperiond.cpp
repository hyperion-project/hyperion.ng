
// QT includes
#include <QCoreApplication>

// Json-Schema includes
#include <utils/jsonschema/JsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>

// Dispmanx grabber includes
#include <dispmanx-grabber/DispmanxWrapper.h>

// JsonServer includes
#include <jsonserver/JsonServer.h>

int main(int argc, char** argv)
{
	// Initialising QCoreApplication
	QCoreApplication app(argc, argv);
	std::cout << "QCoreApplication initialised" << std::endl;

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

	DispmanxWrapper dispmanx(64, 64, 10, &hyperion);
	dispmanx.start();
	std::cout << "Frame grabber created and started" << std::endl;

	JsonServer jsonServer(&hyperion);
	std::cout << "Json server created and started on port " << jsonServer.getPort() << std::endl;

	app.exec();
	std::cout << "Application closed" << std::endl;
}
