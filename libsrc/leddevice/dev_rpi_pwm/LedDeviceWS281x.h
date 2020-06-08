#pragma once

// LedDevice includes
#include <leddevice/LedDevice.h>
#include <ws2811.h>

///
/// Implementation of the LedDevice interface for writing to Ws2812 led device via pwm.
///
class LedDeviceWS281x : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	explicit LedDeviceWS281x(const QJsonObject &deviceConfig);

	///
	/// Destructor of the LedDevice, waits for DMA to complete and then cleans up
	///
	virtual ~LedDeviceWS281x() override;

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	virtual bool init(const QJsonObject &deviceConfig) override;

public slots:
	///
	/// Closes the output device.
	/// Includes switching-off the device and stopping refreshes
	///
	virtual int close() override;

protected:
	///
	/// Writes the led color values to the led-device
	///
	/// @param ledValues The color-value per led
	/// @return Zero on succes else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues) override;

private:

	ws2811_t    _led_string;
	int         _channel;
	RGBW::WhiteAlgorithm _whiteAlgorithm;
	ColorRgbw   _temp_rgbw;
};
