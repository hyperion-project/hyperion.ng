#include <exception>

#include "LedDeviceWS281x.h"

LedDeviceWS281x::LedDeviceWS281x(const QJsonObject &deviceConfig)
	: LedDevice()
{
	_deviceReady = init(deviceConfig);
}

LedDeviceWS281x::~LedDeviceWS281x()
{
	if (_deviceReady)
	{
		ws2811_fini(&_led_string);
	}
}

bool LedDeviceWS281x::init(const QJsonObject &deviceConfig)
{
	LedDevice::init(deviceConfig);

	QString whiteAlgorithm = deviceConfig["whiteAlgorithm"].toString("white_off");
	_whiteAlgorithm            = RGBW::stringToWhiteAlgorithm(whiteAlgorithm);
	Debug( _log, "whiteAlgorithm : %s", QSTRING_CSTR(whiteAlgorithm));
	Debug( _log, "rgbw : %d", deviceConfig["rgbw"].toBool(false) );
	if (_whiteAlgorithm == RGBW::INVALID)
	{
		Error(_log, "unknown whiteAlgorithm %s", QSTRING_CSTR(whiteAlgorithm));
		return false;
	}

	_channel           = deviceConfig["pwmchannel"].toInt(0);
	if (_channel != 0 && _channel != 1)
	{
		throw std::runtime_error("WS281x: invalid PWM channel; must be 0 or 1.");
	}

	memset(&_led_string, 0, sizeof(_led_string));
	_led_string.freq   = deviceConfig["freq"].toInt(800000ul);
	_led_string.dmanum = deviceConfig["dma"].toInt(5);
	_led_string.channel[_channel].gpionum    = deviceConfig["gpio"].toInt(18);
	_led_string.channel[_channel].count      = deviceConfig["leds"].toInt(256);
	_led_string.channel[_channel].invert     = deviceConfig["invert"].toInt(0);
	_led_string.channel[_channel].strip_type = (deviceConfig["rgbw"].toBool(false) ? SK6812_STRIP_GRBW : WS2811_STRIP_RGB);
	_led_string.channel[_channel].brightness = 255;

	_led_string.channel[!_channel].gpionum = 0;
	_led_string.channel[!_channel].invert = _led_string.channel[_channel].invert;
	_led_string.channel[!_channel].count = 0;
	_led_string.channel[!_channel].brightness = 0;
	_led_string.channel[!_channel].strip_type = WS2811_STRIP_RGB;

	Debug( _log, "ws281x strip type : %d", _led_string.channel[_channel].strip_type );

	if (ws2811_init(&_led_string) < 0)
	{
		throw std::runtime_error("Unable to initialize ws281x library.");
	}
	
	return true;
}

LedDevice* LedDeviceWS281x::construct(const QJsonObject &deviceConfig)
{
	return new LedDeviceWS281x(deviceConfig);
}

// Send new values down the LED chain
int LedDeviceWS281x::write(const std::vector<ColorRgb> &ledValues)
{
	int idx = 0;
	for (const ColorRgb& color : ledValues)
	{
		if (idx >= _led_string.channel[_channel].count)
		{
			break;
		}

		_temp_rgbw.red = color.red;
		_temp_rgbw.green = color.green;
		_temp_rgbw.blue = color.blue;
		_temp_rgbw.white = 0;

		if (_led_string.channel[_channel].strip_type == SK6812_STRIP_GRBW)
		{
			Rgb_to_Rgbw(color, &_temp_rgbw, _whiteAlgorithm);
		}

		_led_string.channel[_channel].leds[idx++] = 
			((uint32_t)_temp_rgbw.white << 24) + ((uint32_t)_temp_rgbw.red << 16) + ((uint32_t)_temp_rgbw.green << 8) + _temp_rgbw.blue;

	}
	while (idx < _led_string.channel[_channel].count)
	{
		_led_string.channel[_channel].leds[idx++] = 0;
	}
	
	return ws2811_render(&_led_string) ? -1 : 0;
}
