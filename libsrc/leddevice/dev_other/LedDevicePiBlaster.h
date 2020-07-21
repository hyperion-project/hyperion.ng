#ifndef LEDEVICEPIBLASTER_H
#define LEDEVICEPIBLASTER_H

// LedDevice includes
#include <leddevice/LedDevice.h>

///
/// Implementation of the LedDevice interface for writing to pi-blaster based PWM LEDs
///
class LedDevicePiBlaster : public LedDevice
{
public:
	///
	/// @brief Constructs a pi-Blaster LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDevicePiBlaster(const QJsonObject &deviceConfig);

	///
	/// @brief Destructor of the LedDevice
	///
	virtual ~LedDevicePiBlaster() override;

	///
	/// @brief Constructs the LED-device
	///
	/// @param[in] deviceConfig Device's configuration as JSON-Object
	/// @return LedDevice constructed
	static LedDevice* construct(const QJsonObject &deviceConfig);

protected:

	///
	/// @brief Initialise the device's configuration
	///
	/// @param[in] deviceConfig the JSON device configuration
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// Attempts to open the piblaster-device. This will only succeed if the device is not yet open
	/// and the device is available.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	virtual int open() override;

	///
	/// @brief Closes the output device.
	///
	/// @return Zero on success (i.e. device is closed), else negative
	///
	virtual int close() override;

private:

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> &ledValues) override;

	/// The name of the output device (very likely '/dev/pi-blaster')
	QString _deviceName;

	int _gpio_to_led[64];
	char _gpio_to_color[64];

	/// File-Pointer to the PiBlaster device
	FILE * _fid;

};

#endif // LEDEVICETEMPLATE_H
