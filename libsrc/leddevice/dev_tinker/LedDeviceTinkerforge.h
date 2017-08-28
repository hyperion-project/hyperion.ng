#pragma once

// STL includes
#include <cstdio>

#include <QString>

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
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceTinkerforge(const QJsonObject &deviceConfig);

	virtual ~LedDeviceTinkerforge();

	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool init(const QJsonObject &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const QJsonObject &deviceConfig);

	///
	/// Attempts to open a connection to the master bricklet and the led strip bricklet.
	///
	/// @return Zero on succes else negative
	///
	int open();

private:
	///
	/// Writes the colors to the led strip bricklet
	///
	/// @param ledValues The color value for each led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> &ledValues);

	///
	/// Writes the data to the led strip blicklet 
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
