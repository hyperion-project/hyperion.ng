#pragma once

// STL includes
#include <fstream>
#include <string>
#include <QUdpSocket>
#include <QStringList>
// Leddevice includes
#include <leddevice/LedDevice.h>

///
class LedDeviceTpm2net : public LedDevice
{
public:
	///
	/// Constructs specific LedDevice
	///
	/// @param deviceConfig json device config
	///
	LedDeviceTpm2net(const Json::Value &deviceConfig);

	virtual ~LedDeviceTpm2net();
	
	///
	/// Sets configuration
	///
	/// @param deviceConfig the json device config
	/// @return true if success
	bool setConfig(const Json::Value &deviceConfig);

	/// constructs leddevice
	static LedDevice* construct(const Json::Value &deviceConfig);

	///
	/// Writes the given led-color values to the output stream
	///
	/// @param ledValues The color-value per led
	///
	/// @return Zero on success else negative
	///
	virtual int write(const std::vector<ColorRgb> & ledValues);

	/// Switch the leds off
	virtual int switchOff();

private:
	/// The host of the master brick
	QHostAddress _host;

	/// The port of the master brick
	uint16_t _port;
	QUdpSocket _socket;
};
