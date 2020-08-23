#ifndef LEDEVICETINKERFORGE_H
#define LEDEVICETINKERFORGE_H

// STL includes
#include <cstdio>

// Hyperion-Leddevice includes
#include <leddevice/LedDevice.h>

extern "C" {
	#include <tinkerforge/ip_connection.h>
	#include <tinkerforge/bricklet_led_strip.h>
}

class LedDeviceTinkerforge : public LedDevice
{
public:

	///
	/// @brief Constructs a Tinkerforge LED-device
	///
	/// @param deviceConfig Device's configuration as JSON-Object
	///
	explicit LedDeviceTinkerforge(const QJsonObject &deviceConfig);

	~LedDeviceTinkerforge() override;

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
	/// @return True, if success
	///
	bool init(const QJsonObject &deviceConfig) override;

	///
	/// @brief Opens a connection to the master bricklet and the led strip bricklet.
	///
	/// @return Zero on success (i.e. device is ready), else negative
	///
	int open() override;

	///
	/// @brief Writes the RGB-Color values to the LEDs.
	///
	/// @param[in] ledValues The RGB-color per LED
	/// @return Zero on success, else negative
	///
	int write(const std::vector<ColorRgb> & ledValues) override;

private:

	///
	/// Writes the data to the LED strip bricklet
	int transferLedData(LEDStrip *ledstrip, unsigned int index, unsigned int length, uint8_t *redChannel, uint8_t *greenChannel, uint8_t *blueChannel);

	/// The host of the master brick
	QString _host;

	/// The port of the master brick
	uint16_t _port;

	/// The uid of the led strip bricklet
	QString _uid;

	/// The interval/rate
	unsigned _interval;

	/// ip connection handle 
	IPConnection *_ipConnection;

	/// led strip handle
	LEDStrip *_ledStrip;

	/// buffer for red channel led data
	std::vector<uint8_t> _redChannel;

	/// buffer for red channel led data
	std::vector<uint8_t> _greenChannel;

	/// buffer for red channel led data
	std::vector<uint8_t> _blueChannel;

	/// buffer size of the color channels
	unsigned int _colorChannelSize;

};

#endif // LEDEVICETINKERFORGE_H
