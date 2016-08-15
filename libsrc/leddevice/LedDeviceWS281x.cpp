#include <iostream>
#include <exception>

#include "LedDeviceWS281x.h"

// Constructor
LedDeviceWS281x::LedDeviceWS281x(const int gpio, const int leds, const uint32_t freq, const int dmanum, const int pwmchannel, const int invert, const int rgbw, const std::string& whiteAlgorithm)
	: LedDevice()
	, _channel(pwmchannel)
	, _initialized(false)
	, _whiteAlgorithm(whiteAlgorithm)
{
	Debug( _log, "whiteAlgorithm : %s", whiteAlgorithm.c_str());

	_led_string.freq = freq;
	_led_string.dmanum = dmanum;
	if (pwmchannel != 0 && pwmchannel != 1)
	{
		throw std::runtime_error("WS281x: invalid PWM channel; must be 0 or 1.");
	}
	_led_string.channel[_channel].gpionum = gpio;
	_led_string.channel[_channel].invert = invert;
	_led_string.channel[_channel].count = leds;
	_led_string.channel[_channel].brightness = 255;
	_led_string.channel[_channel].strip_type = ((rgbw == 1) ? SK6812_STRIP_GRBW : WS2811_STRIP_RGB);

	_led_string.channel[!_channel].gpionum = 0;
	_led_string.channel[!_channel].invert = invert;
	_led_string.channel[!_channel].count = 0;
	_led_string.channel[!_channel].brightness = 0;
	_led_string.channel[!_channel].strip_type = WS2811_STRIP_RGB;
	if (ws2811_init(&_led_string) < 0)
	{
		throw std::runtime_error("Unable to initialize ws281x library.");
	}
	_initialized = true;
}

// Send new values down the LED chain
int LedDeviceWS281x::write(const std::vector<ColorRgb> &ledValues)
{
	if (!_initialized)
		return -1;

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

// Turn off the LEDs by sending 000000's
// TODO Allow optional power switch out another gpio, if this code handles it can
// make it more likely we don't accidentally drive data into an off strip
int LedDeviceWS281x::switchOff()
{
	if (!_initialized)
	{
		return -1;
	}

	int idx = 0;
	while (idx < _led_string.channel[_channel].count)
		_led_string.channel[_channel].leds[idx++] = 0;

	return ws2811_render(&_led_string) ? -1 : 0;
}

// Destructor
LedDeviceWS281x::~LedDeviceWS281x()
{
	if (_initialized)
	{
		ws2811_fini(&_led_string);
	}
	_initialized = false;
}
