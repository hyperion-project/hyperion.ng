
// Syslog include
#include <syslog.h>

// JsonSchema include
#include <utils/jsonschema/JsonFactory.h>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/LedDevice.h>

#include "LedDeviceWs2801.h"

LedDevice* constructDevice(const Json::Value& deviceConfig)
{
	std::cout << "Device configuration: " << deviceConfig << std::endl;
	LedDevice* device = nullptr;
	if (deviceConfig["type"].asString() == "ws2801")
	{
		const std::string name = "WS-2801";
		const std::string output = deviceConfig["output"].asString();
		const unsigned interval  = deviceConfig["interval"].asInt();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceWs2801* deviceWs2801 = new LedDeviceWs2801(name, output, interval, rate);
		deviceWs2801->open();

		device = deviceWs2801;
	}
	else
	{
		// Unknown / Unimplemented device
	}
	return device;
}

Hyperion::Hyperion(const Json::Value &jsonConfig) :
	mLedString(LedString::construct(jsonConfig["leds"], jsonConfig["color"])),
	mImage(nullptr),
	mDevice(constructDevice(jsonConfig["device"]))
{
	// empty
}


Hyperion::~Hyperion()
{
	// Delete the existing image (or delete nullptr)
	delete mImage;

	// Delete the Led-String
	delete mDevice;
}

void Hyperion::setInputSize(const unsigned width, const unsigned height)
{
	// Delete the existing image (or delete nullptr)
	delete mImage;

	// Create the new image with the mapping to the leds
	mImage = new RgbImage(width, height);
	mLedsMap.createMapping(*mImage, mLedString.leds());
}

void Hyperion::commit()
{
	// Derive the color per led
	std::vector<RgbColor> ledColors = mLedsMap.getMeanLedColor();
//	const std::vector<RgbColor> ledColors = mLedsMap.getMedianLedColor();

	// Write the Led colors to the led-string
	mDevice->write(ledColors);
}

void Hyperion::operator() (const RgbImage& inputImage)
{
	// Copy the input-image into the buffer
	mImage->copy(inputImage);

	// Derive the color per led
	std::vector<RgbColor> ledColors = mLedsMap.getMeanLedColor();
//	std::vector<RgbColor> ledColors = mLedsMap.getMedianLedColor();
	applyTransform(ledColors);

	// Write the Led colors to the led-string
	mDevice->write(ledColors);
}

void Hyperion::setColor(const RgbColor& color)
{
	mDevice->write(std::vector<RgbColor>(mLedString.leds().size(), color));
}

void Hyperion::applyTransform(std::vector<RgbColor>& colors) const
{
	for (RgbColor& color : colors)
	{
		color.red   = (color.red  < mLedString.red.blacklevel)?    0 : mLedString.red.adjust   + mLedString.red.gamma   * color.red;
		color.green = (color.green < mLedString.green.blacklevel)? 0 : mLedString.green.adjust + mLedString.green.gamma * color.green;
		color.blue  = (color.blue  < mLedString.blue.blacklevel)?  0 : mLedString.blue.adjust  + mLedString.blue.gamma  * color.blue;
	}
}
