
// Syslog include
#include <syslog.h>

#include <QDateTime>

// JsonSchema include
#include <utils/jsonschema/JsonFactory.h>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/LedDevice.h>
#include <hyperion/ImageProcessorFactory.h>

#include "LedDeviceWs2801.h"
#include "LedDeviceTest.h"
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
	else if (deviceConfig["type"].asString() == "test")
	{
		device = new LedDeviceTest();
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
LedString Hyperion::createLedString(const Json::Value& ledsConfig)
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
	_ledString(createLedString(jsonConfig["leds"])),
	_muxer(_ledString.leds().size()),
	_redTransform(createColorTransform(jsonConfig["color"]["red"])),
	_greenTransform(createColorTransform(jsonConfig["color"]["green"])),
	_blueTransform(createColorTransform(jsonConfig["color"]["blue"])),
	_device(constructDevice(jsonConfig["device"])),
	_timer()
{
	ImageProcessorFactory::getInstance().init(_ledString);

	_timer.setSingleShot(true);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

	// initialize the leds
	update();
}


Hyperion::~Hyperion()
{
	// Delete the Led-String
	delete _device;

	// Delete the color-transform
	delete _blueTransform;
	delete _greenTransform;
	delete _redTransform;
}

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}

void Hyperion::setColor(int priority, RgbColor & color, const int timeout_ms)
{
	// create led output
	std::vector<RgbColor> ledColors(_ledString.leds().size(), color);

	// set colors
	setColors(priority, ledColors, timeout_ms);
}

void Hyperion::setColors(int priority, std::vector<RgbColor>& ledColors, const int timeout_ms)
{
	// Apply the transform to each led and color-channel
	for (RgbColor& color : ledColors)
	{
		color.red   = _redTransform->transform(color.red);
		color.green = _greenTransform->transform(color.green);
		color.blue  = _blueTransform->transform(color.blue);
	}

	if (timeout_ms > 0)
	{
		const uint64_t timeoutTime = QDateTime::currentMSecsSinceEpoch() + timeout_ms;
		_muxer.setInput(priority, ledColors, timeoutTime);
	}
	else
	{
		_muxer.setInput(priority, ledColors);
	}

	if (priority == _muxer.getCurrentPriority())
	{
		update();
	}
}

void Hyperion::clear(int priority)
{
	if (_muxer.hasPriority(priority))
	{
		_muxer.clearInput(priority);

		// update leds if necessary
		if (priority < _muxer.getCurrentPriority());
		{
			update();
		}
	}
}

void Hyperion::clearall()
{
	_muxer.clearAll();

	// update leds
	update();
}

void Hyperion::update()
{
	// Update the muxer, cleaning obsolete priorities
	_muxer.setCurrentTime(QDateTime::currentMSecsSinceEpoch());

	// Obtain the current priority channel
	int priority = _muxer.getCurrentPriority();
	const PriorityMuxer::InputInfo & priorityInfo  = _muxer.getInputInfo(priority);

	// Write the data to the device
	_device->write(priorityInfo.ledColors);

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
