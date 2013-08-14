
// Syslog include
#include <syslog.h>

#include <QDateTime>

// JsonSchema include
#include <utils/jsonschema/JsonFactory.h>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/LedDevice.h>

#include "LedDeviceWs2801.h"
#include "ColorTransform.h"

using namespace hyperion;

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

ColorTransform* createColorTransform(const Json::Value& colorConfig)
{
	const double threshold  = colorConfig["threshold"].asDouble();
	const double gamma      = colorConfig["gamma"].asDouble();
	const double blacklevel = colorConfig["blacklevel"].asDouble();
	const double whitelevel = colorConfig["whitelevel"].asDouble();

	ColorTransform* transform = new ColorTransform(threshold, gamma, blacklevel, whitelevel);
	return transform;
}
LedString createLedString(const Json::Value& ledsConfig)
{
	LedString ledString;

	for (const Json::Value& ledConfig : ledsConfig)
	{
		Led led;
		led.index = ledConfig["index"].asInt();
		const Json::Value& hscanConfig = ledConfig["hscan"];
		const Json::Value& vscanConfig = ledConfig["vscan"];
		led.minX_frac = std::max(0.0, std::min(100.0, hscanConfig["minimum"].asDouble()))/100.0;
		led.maxX_frac = std::max(0.0, std::min(100.0, hscanConfig["maximum"].asDouble()))/100.0;
		led.minY_frac = 1.0 - std::max(0.0, std::min(100.0, vscanConfig["maximum"].asDouble()))/100.0;
		led.maxY_frac = 1.0 - std::max(0.0, std::min(100.0, vscanConfig["minimum"].asDouble()))/100.0;

		ledString.leds().push_back(led);
	}
	return ledString;
}

Hyperion::Hyperion(const Json::Value &jsonConfig) :
	mLedString(createLedString(jsonConfig["leds"])),
	mRedTransform(  createColorTransform(jsonConfig["color"]["red"])),
	mGreenTransform(createColorTransform(jsonConfig["color"]["green"])),
	mBlueTransform( createColorTransform(jsonConfig["color"]["blue"])),
	mDevice(constructDevice(jsonConfig["device"])),
	_timer()
{
	_timer.setSingleShot(true);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

}


Hyperion::~Hyperion()
{
	// Delete the Led-String
	delete mDevice;

	// Delete the color-transform
	delete mBlueTransform;
	delete mGreenTransform;
	delete mRedTransform;
}

unsigned Hyperion::getLedCount() const
{
	return mLedString.leds().size();
}

void Hyperion::setValue(int priority, std::vector<RgbColor>& ledColors, const int timeout_ms)
{
	// Apply the transform to each led and color-channel
	for (RgbColor& color : ledColors)
	{
		color.red   = mRedTransform->transform(color.red);
		color.green = mGreenTransform->transform(color.green);
		color.blue  = mBlueTransform->transform(color.blue);
	}

	if (timeout_ms > 0)
	{
		const uint64_t timeoutTime = QDateTime::currentMSecsSinceEpoch() + timeout_ms;
		mMuxer.setInput(priority, ledColors, timeoutTime);
	}
	else
	{
		mMuxer.setInput(priority, ledColors);
	}

	if (priority == mMuxer.getCurrentPriority())
	{
		update();
	}
}

void Hyperion::update()
{
	// Update the muxer, cleaning obsolete priorities
	mMuxer.setCurrentTime(QDateTime::currentMSecsSinceEpoch());

	// Obtain the current priority channel
	int priority = mMuxer.getCurrentPriority();
	const PriorityMuxer::InputInfo & priorityInfo  = mMuxer.getInputInfo(priority);

	// Write the data to the device
	mDevice->write(priorityInfo.ledColors);

	// Start the timeout-timer
	if (priorityInfo.timeoutTime_ms == -1)
	{
		_timer.stop();
	}
	else
	{
		int timeout_ms = std::max(0, int(priorityInfo.timeoutTime_ms - QDateTime::currentMSecsSinceEpoch()));
		_timer.start(timeout_ms);
	}
}
