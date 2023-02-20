
// Utils includes
#include <utils/Image.h>
#include <utils/jsonschema/QJsonFactory.h>
#include <utils/Logger.h>

// Hyperion includes
#include <utils/hyperion.h>
#include <hyperion/ImageToLedsMap.h>

int main()
{
	Logger* log = Logger::getInstance("TestImageLedsMap");
	Logger::setLogLevel(Logger::DEBUG);

	const QString schemaFile = ":/hyperion-schema";
	const QString configFile = ":/hyperion_default.config";


	QJsonObject config;
	if (QJsonFactory::load(schemaFile, configFile, config) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}

	const LedString ledString = hyperion::createLedString(config["leds"].toArray(), hyperion::createColorOrder(config["device"].toObject()));

	const ColorRgb testColor = {64, 123, 12};

	Image<ColorRgb> image(64, 64, testColor);
	hyperion::ImageToLedsMap map(log, 64, 64, 0, 0, ledString.leds());

	std::vector<ColorRgb> ledColors(ledString.leds().size());
	map.getMeanLedColor(image, ledColors);

	std::cout << "[";
	for (const ColorRgb & color : ledColors)
	{
		std::cout << color;
	}
	std::cout << "]" << std::endl;

	return 0;
}
