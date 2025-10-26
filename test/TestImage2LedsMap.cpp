
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
	Logger::setLogLevel(Logger::LOG_DEBUG);

	const QString schemaFile = ":/hyperion-schema";
	const QString configFile = ":/hyperion_default.config";


	QJsonObject config;
	if (QJsonFactory::load(schemaFile, configFile, config) < 0)
	{
		std::cerr << "UNABLE TO LOAD CONFIGURATION" << '\n';
		return -1;
	}

	QJsonArray const ledLayout = config["leds"].toArray();
	ColorOrder const colorOrder = hyperion::createColorOrder(config["device"].toObject()["colorOrder"].toString("rgb"));
	const LedString ledString = LedString::createLedString(ledLayout, colorOrder, static_cast<int>(ledLayout.size()) );

	const ColorRgb testColor = {64, 123, 12};

	Image<ColorRgb> const image(64, 64, testColor);
	hyperion::ImageToLedsMap const map(log, 64, 64, 0, 0, ledString.leds());

	QVector<ColorRgb> ledColors(ledString.leds().size());
	map.getMeanLedColor(image, ledColors);

	std::cout << "[";
	for (const ColorRgb & color : ledColors)
	{
		std::cout << color;
	}
	std::cout << "]" << '\n';

	return 0;
}
