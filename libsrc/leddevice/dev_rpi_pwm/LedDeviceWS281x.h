#ifndef LEDEVICEWS281X_H
#define LEDEVICEWS281X_H

// LedDevice includes
#include <leddevice/LedDevice.h>
#include <ws2811.h>

///
/// Implementation of the LedDevice interface for writing to WS281x LED-device via pwm.
///
class LedDeviceWS281x : public LedDevice
{
public:

	///
	/// @brief Constructs an WS281x LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceWS281x(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	~LedDeviceWS281x() override;

	///
	/// @brief Destructor of the LedDevice
	///
	static LedDevice* construct(const QJsonObject &deviceConfig);

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens the output device.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	int close() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

private:

	ws2811_t    _led_string;
	int         _channel;
	RGBW::WhiteAlgorithm _whiteAlgorithm;
	ColorRgbw   _temp_rgbw;
};

#endif // LEDEVICEWS281X_H
