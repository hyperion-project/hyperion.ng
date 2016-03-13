#ifndef LEDDEVICEWS281X_H_
#define LEDDEVICEWS281X_H_

#pragma once

#include <leddevice/LedDevice.h>
#include <ws2811.h>

class LedDeviceWS281x : public LedDevice
{
public:
	///
	/// Constructs the LedDevice for WS281x (one wire 800kHz)
	///
	/// @param gpio   The gpio pin to use (BCM chip counting, default is 18)
	/// @param leds   The number of leds attached to the gpio pin
	/// @param freq   The target frequency for the data line, default is 800000
	/// @param dmanum The DMA channel to use, default is 5
	///
	LedDeviceWS281x(const int gpio, const int leds, const uint32_t freq, int dmanum);

	///
	/// Destructor of the LedDevice, waits for DMA to complete and then cleans up
	///
	~LedDeviceWS281x();

	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	ws2811_t led_string;
	bool initialized;
};

#endif /* LEDDEVICEWS281X_H_ */
