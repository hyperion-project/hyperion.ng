
// QT includes
#include <QDateTime>

// JsonSchema include
#include <utils/jsonschema/JsonFactory.h>

// hyperion include
#include <hyperion/Hyperion.h>
#include <hyperion/LedDevice.h>
#include <hyperion/ImageProcessorFactory.h>

#include "LedDeviceWs2801.h"
#include "LedDeviceTest.h"

#include "LinearColorSmoothing.h"

#include <utils/ColorTransform.h>
#include <utils/HsvTransform.h>

LedDevice* Hyperion::createDevice(const Json::Value& deviceConfig)
{
	std::cout << "Device configuration: " << deviceConfig << std::endl;
	LedDevice* device = nullptr;
	if (deviceConfig["type"].asString() == "ws2801")
	{
		const std::string output = deviceConfig["output"].asString();
		const unsigned rate      = deviceConfig["rate"].asInt();

		LedDeviceWs2801* deviceWs2801 = new LedDeviceWs2801(output, rate);
		deviceWs2801->open();

		device = deviceWs2801;
	}
	else if (deviceConfig["type"].asString() == "test")
	{
		device = new LedDeviceTest();
	}
	else
	{
		std::cout << "Unable to create device" << std::endl;
		// Unknown / Unimplemented device
	}
	return device;
}

HsvTransform * Hyperion::createHsvTransform(const Json::Value & hsvConfig)
{
	const double saturationGain = hsvConfig.get("saturationGain", 1.0).asDouble();
	const double valueGain      = hsvConfig.get("valueGain",      1.0).asDouble();

	return new HsvTransform(saturationGain, valueGain);
}

ColorTransform* Hyperion::createColorTransform(const Json::Value& colorConfig)
{
	const double threshold  = colorConfig.get("threshold", 0.0).asDouble();
	const double gamma      = colorConfig.get("gamma", 1.0).asDouble();
	const double blacklevel = colorConfig.get("blacklevel", 0.0).asDouble();
	const double whitelevel = colorConfig.get("whitelevel", 1.0).asDouble();

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
		led.minX_frac = std::max(0.0, std::min(1.0, hscanConfig["minimum"].asDouble()));
		led.maxX_frac = std::max(0.0, std::min(1.0, hscanConfig["maximum"].asDouble()));
		led.minY_frac = std::max(0.0, std::min(1.0, vscanConfig["minimum"].asDouble()));
		led.maxY_frac = std::max(0.0, std::min(1.0, vscanConfig["maximum"].asDouble()));

		// Fix if the user swapped min and max
		if (led.minX_frac > led.maxX_frac)
		{
			std::swap(led.minX_frac, led.maxX_frac);
		}
		if (led.minY_frac > led.maxY_frac)
		{
			std::swap(led.minY_frac, led.maxY_frac);
		}

		ledString.leds().push_back(led);
	}
	return ledString;
}

LedDevice * Hyperion::createColorSmoothing(const Json::Value & smoothingConfig, LedDevice * ledDevice)
{
	//return new LinearColorSmoothing(ledDevice, 20.0, .3);
	return ledDevice;
}

Hyperion::Hyperion(const Json::Value &jsonConfig) :
	_ledString(createLedString(jsonConfig["leds"])),
	_muxer(_ledString.leds().size()),
	_hsvTransform(createHsvTransform(jsonConfig["color"]["hsv"])),
	_redTransform(createColorTransform(jsonConfig["color"]["red"])),
	_greenTransform(createColorTransform(jsonConfig["color"]["green"])),
	_blueTransform(createColorTransform(jsonConfig["color"]["blue"])),
	_haveBgrOutput(jsonConfig["device"].get("bgr-output", false).asBool()),
	_device(createDevice(jsonConfig["device"])),
	_timer()
{
	// initialize the image processor factory
	ImageProcessorFactory::getInstance().init(_ledString, jsonConfig["blackborderdetector"].get("enable", true).asBool());

	// initialize the color smoothing filter
	_device = createColorSmoothing(jsonConfig["smoothing"], _device);

	// setup the timer
	_timer.setSingleShot(true);
	QObject::connect(&_timer, SIGNAL(timeout()), this, SLOT(update()));

	// initialize the leds
	update();
}


Hyperion::~Hyperion()
{
	// switch off all leds
	clearall();
	_device->switchOff();

	// Delete the Led-String
	delete _device;

	// delete he hsv transform
	delete _hsvTransform;

	// Delete the color-transform
	delete _blueTransform;
	delete _greenTransform;
	delete _redTransform;
}

unsigned Hyperion::getLedCount() const
{
	return _ledString.leds().size();
}

void Hyperion::setColor(int priority, const RgbColor &color, const int timeout_ms)
{
	// create led output
	std::vector<RgbColor> ledColors(_ledString.leds().size(), color);

	// set colors
	setColors(priority, ledColors, timeout_ms);
}

void Hyperion::setColors(int priority, const std::vector<RgbColor>& ledColors, const int timeout_ms)
{
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

void Hyperion::setTransform(Hyperion::Transform transform, Hyperion::Color color, double value)
{
	// select the transform of the requested color
	ColorTransform * t = nullptr;
	switch (color)
	{
	case RED:
		t = _redTransform;
		break;
	case GREEN:
		t = _greenTransform;
		break;
	case BLUE:
		t = _blueTransform;
		break;
	default:
		break;
	}

	// set transform value
	switch (transform)
	{
	case SATURATION_GAIN:
		_hsvTransform->setSaturationGain(value);
		break;
	case VALUE_GAIN:
		_hsvTransform->setValueGain(value);
		break;
	case THRESHOLD:
		assert (t != nullptr);
		t->setThreshold(value);
		break;
	case GAMMA:
		assert (t != nullptr);
		t->setGamma(value);
		break;
	case BLACKLEVEL:
		assert (t != nullptr);
		t->setBlacklevel(value);
		break;
	case WHITELEVEL:
		assert (t != nullptr);
		t->setWhitelevel(value);
		break;
	default:
		assert(false);
	}

	// update the led output
	update();
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

double Hyperion::getTransform(Hyperion::Transform transform, Hyperion::Color color) const
{
	// select the transform of the requested color
	ColorTransform * t = nullptr;
	switch (color)
	{
	case RED:
		t = _redTransform;
		break;
	case GREEN:
		t = _greenTransform;
		break;
	case BLUE:
		t = _blueTransform;
		break;
	default:
		break;
	}

	// set transform value
	switch (transform)
	{
	case SATURATION_GAIN:
		return _hsvTransform->getSaturationGain();
	case VALUE_GAIN:
		return _hsvTransform->getValueGain();
	case THRESHOLD:
		assert (t != nullptr);
		return t->getThreshold();
	case GAMMA:
		assert (t != nullptr);
		return t->getGamma();
	case BLACKLEVEL:
		assert (t != nullptr);
		return t->getBlacklevel();
	case WHITELEVEL:
		assert (t != nullptr);
		return t->getWhitelevel();
	default:
		assert(false);
	}

	return 999.0;
}

QList<int> Hyperion::getActivePriorities() const
{
	return _muxer.getPriorities();
}

const Hyperion::InputInfo &Hyperion::getPriorityInfo(const int priority) const
{
	return _muxer.getInputInfo(priority);
}

void Hyperion::update()
{
	// Update the muxer, cleaning obsolete priorities
	_muxer.setCurrentTime(QDateTime::currentMSecsSinceEpoch());

	// Obtain the current priority channel
	int priority = _muxer.getCurrentPriority();
	const PriorityMuxer::InputInfo & priorityInfo  = _muxer.getInputInfo(priority);

	// Apply the transform to each led and color-channel
	std::vector<RgbColor> ledColors(priorityInfo.ledColors);
	for (RgbColor& color : ledColors)
	{
		_hsvTransform->transform(color.red, color.green, color.blue);
		color.red   = _redTransform->transform(color.red);
		color.green = _greenTransform->transform(color.green);
		color.blue  = _blueTransform->transform(color.blue);

		if (_haveBgrOutput)
		{
			std::swap(color.red, color.blue);
		}
	}

	// Write the data to the device
	_device->write(ledColors);

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
