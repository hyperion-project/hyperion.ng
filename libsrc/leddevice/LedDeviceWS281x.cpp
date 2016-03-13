#include <iostream>

#include "LedDeviceWS281x.h"

// Constructor
LedDeviceWS281x::LedDeviceWS281x(const int gpio, const int leds, const uint32_t freq, const int dmanum)
{
	initialized = false;
	led_string.freq = freq;
	led_string.dmanum = dmanum;
	led_string.channel[0].gpionum = gpio;
	led_string.channel[0].invert = 0;
	led_string.channel[0].count = leds;
	led_string.channel[0].brightness = 255;
	led_string.channel[0].strip_type = WS2811_STRIP_RGB;

	led_string.channel[1].gpionum = 0;
	led_string.channel[1].invert = 0;
	led_string.channel[1].count = 0;
	led_string.channel[1].brightness = 0;
	led_string.channel[0].strip_type = WS2811_STRIP_RGB;
	if (ws2811_init(&led_string) < 0) {
		std::cout << "Unable to initialize ws281x library." << std::endl;
		throw -1;
	}
	initialized = true;
}

// Send new values down the LED chain
int LedDeviceWS281x::write(const std::vector<ColorRgb> &ledValues)
{
	if (!initialized)
		return -1;

	int idx = 0;
	for (const ColorRgb& color : ledValues)
	{
		if (idx >= led_string.channel[0].count)
			break;
		led_string.channel[0].leds[idx++] = ((uint32_t)color.red << 16) + ((uint32_t)color.green << 8) + color.blue;
	}
	while (idx < led_string.channel[0].count)
		led_string.channel[0].leds[idx++] = 0;

	if (ws2811_render(&led_string))
		return -1;

	return 0;
}

// Turn off the LEDs by sending 000000's
// TODO Allow optional power switch out another gpio, if this code handles it can
// make it more likely we don't accidentally drive data into an off strip
int LedDeviceWS281x::switchOff()
{
	if (!initialized)
		return -1;

	int idx = 0;
	while (idx < led_string.channel[0].count)
		led_string.channel[0].leds[idx++] = 0;

	if (ws2811_render(&led_string))
		return -1;

	return 0;
}

// Destructor
LedDeviceWS281x::~LedDeviceWS281x()
{
	if (initialized)
	{
		std::cout << "Shutdown WS281x PWM and DMA channel" << std::endl;
		ws2811_fini(&led_string);
	}
	initialized = false;
}
