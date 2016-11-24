
// Utils includes
#include <utils/Image.h>
#include <utils/jsonschema/QJsonFactory.h>

// Hyperion includes
#include <hyperion/Hyperion.h>
#include <hyperion/ImageToLedsMap.h>

using namespace hyperion;

int main()
{
	QString homeDir = getenv("RASPILIGHT_HOME");

	const QString schemaFile = homeDir + "/hyperion.schema.json";
	const QString configFile = homeDir + "/hyperion.config.json";

	QJsonObject config;
	if (QJsonFactory::load(schemaFile, configFile, config) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << std::endl;
		return -1;
	}

	const LedString ledString = Hyperion::createLedString(config["leds"], Hyperion::createColorOrder(config["device"].toObject()));

	const ColorRgb testColor = {64, 123, 12};

	Image<ColorRgb> image(64, 64, testColor);
	ImageToLedsMap map(64, 64, 0, 0, ledString.leds());

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
